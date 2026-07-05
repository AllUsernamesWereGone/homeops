#!/usr/bin/env bash
set -Eeuo pipefail

APP_DIR="${APP_DIR:-$HOME/homeops}"
COMPOSE_DIR="${COMPOSE_DIR:-$APP_DIR/infra/compose}"

BACKEND_SERVICE="${BACKEND_SERVICE:-homeops-backend}"

FRONTEND_PORT="${FRONTEND_PORT:-8080}"
MQTT_PORT="${MQTT_PORT:-1883}"

NO_CACHE="${NO_CACHE:-false}"

if [ -z "${CONTAINER_ENGINE:-}" ]; then
  if command -v podman >/dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
  elif command -v docker >/dev/null 2>&1; then
    CONTAINER_ENGINE="docker"
  else
    echo "ERROR: Neither podman nor docker is installed."
    exit 1
  fi
fi

echo "Deploying HomeOps from $APP_DIR"
echo "Container engine: $CONTAINER_ENGINE"

if ! command -v "$CONTAINER_ENGINE" >/dev/null 2>&1; then
  echo "ERROR: Container engine '$CONTAINER_ENGINE' is not installed or not in PATH."
  exit 1
fi

if [ "$CONTAINER_ENGINE" = "podman" ]; then
  if podman compose version >/dev/null 2>&1; then
    COMPOSE_CMD=(podman compose)
  elif command -v podman-compose >/dev/null 2>&1; then
    COMPOSE_CMD=(podman-compose)
  else
    echo "ERROR: Neither 'podman compose' nor 'podman-compose' is available."
    echo "Install podman-compose or a Compose provider for Podman."
    exit 1
  fi
elif [ "$CONTAINER_ENGINE" = "docker" ]; then
  COMPOSE_CMD=(docker compose)
else
  echo "ERROR: Unsupported container engine: $CONTAINER_ENGINE"
  exit 1
fi

compose() {
  "${COMPOSE_CMD[@]}" "$@"
}

read_env_value() {
  local key="$1"
  local file="$COMPOSE_DIR/.env"

  grep -E "^${key}=" "$file" 2>/dev/null \
    | tail -n 1 \
    | cut -d '=' -f2- \
    | tr -d '\r' \
    || true
}

inspect_value() {
  local container_id="$1"
  local template="$2"
  local value

  value="$("$CONTAINER_ENGINE" inspect --format="$template" "$container_id" 2>/dev/null || true)"

  if [ -n "$value" ] && [ "$value" != "<no value>" ]; then
    echo "$value"
  fi
}

get_container_status() {
  local container_id="$1"
  local value

  value="$(inspect_value "$container_id" '{{.State.Health.Status}}')"
  if [ -n "$value" ]; then
    echo "$value"
    return
  fi

  value="$(inspect_value "$container_id" '{{.State.Healthcheck.Status}}')"
  if [ -n "$value" ]; then
    echo "$value"
    return
  fi

  value="$(inspect_value "$container_id" '{{.State.Status}}')"
  if [ -n "$value" ]; then
    echo "$value"
    return
  fi

  echo "missing"
}

cd "$APP_DIR"

echo "Pulling latest changes..."
git pull --ff-only

echo "Checking compose directory..."
if [ ! -f "$COMPOSE_DIR/docker-compose.yml" ]; then
  echo "ERROR: docker-compose.yml not found in $COMPOSE_DIR"
  exit 1
fi

if [ ! -f "$COMPOSE_DIR/.env" ]; then
  echo "ERROR: .env not found in $COMPOSE_DIR"
  echo "Create it with at least:"
  echo "HOST_IP=<mini-pc-lan-or-tailscale-ip>"
  echo "MQTT_HOST_IP=<mini-pc-lan-or-tailscale-ip>"
  exit 1
fi

HOST_IP_VALUE="$(read_env_value HOST_IP)"
MQTT_HOST_IP_VALUE="$(read_env_value MQTT_HOST_IP)"

if [ -z "$HOST_IP_VALUE" ]; then
  echo "ERROR: HOST_IP is missing in $COMPOSE_DIR/.env"
  exit 1
fi

if [ -z "$MQTT_HOST_IP_VALUE" ]; then
  MQTT_HOST_IP_VALUE="$HOST_IP_VALUE"
fi

cd "$COMPOSE_DIR"

echo "Using Compose command: ${COMPOSE_CMD[*]}"
echo "HOST_IP=$HOST_IP_VALUE"
echo "MQTT_HOST_IP=$MQTT_HOST_IP_VALUE"

echo "Validating Compose config..."
compose config >/dev/null

echo "Stopping old containers and removing orphans..."
compose down --remove-orphans

if [ "$NO_CACHE" = "true" ]; then
  echo "Building containers without cache..."
  compose build --no-cache

  echo "Starting containers..."
  compose up -d --force-recreate
else
  echo "Building and starting containers..."
  compose up -d --build --force-recreate
fi

echo "Removing unused images..."
"$CONTAINER_ENGINE" image prune -f

echo "Current containers:"
compose ps

echo "Resolving backend container..."
BACKEND_CONTAINER_ID="$(compose ps -q "$BACKEND_SERVICE" | head -n 1 || true)"

if [ -z "$BACKEND_CONTAINER_ID" ]; then
  echo "ERROR: Could not find backend container for service '$BACKEND_SERVICE'."
  echo "Available services/containers:"
  compose ps
  exit 1
fi

echo "Waiting for backend healthcheck..."

BACKEND_READY=false

for i in {1..30}; do
  BACKEND_STATUS="$(get_container_status "$BACKEND_CONTAINER_ID")"

  if [ "$BACKEND_STATUS" = "healthy" ]; then
    echo "Backend is healthy."
    BACKEND_READY=true
    break
  fi

  if [ "$BACKEND_STATUS" = "running" ]; then
    echo "Backend container is running, but no healthcheck status is exposed."
    echo "Continuing with HTTP checks."
    BACKEND_READY=true
    break
  fi

  if [ "$BACKEND_STATUS" = "unhealthy" ]; then
    echo "ERROR: Backend became unhealthy."
    compose logs --tail=50 "$BACKEND_SERVICE"
    exit 1
  fi

  echo "Backend status: $BACKEND_STATUS"
  sleep 3
done

if [ "$BACKEND_READY" != "true" ]; then
  BACKEND_STATUS="$(get_container_status "$BACKEND_CONTAINER_ID")"
  echo "ERROR: Backend did not become ready. Final status: $BACKEND_STATUS"
  compose logs --tail=80 "$BACKEND_SERVICE"
  exit 1
fi

echo "Testing backend through frontend proxy..."
curl -fsS "http://${HOST_IP_VALUE}:${FRONTEND_PORT}/api/v1/system/info" >/dev/null || {
  echo "ERROR: Backend API check failed"
  compose logs --tail=80 "$BACKEND_SERVICE"
  exit 1
}

echo "Testing frontend..."
curl -fsS "http://${HOST_IP_VALUE}:${FRONTEND_PORT}/" >/dev/null || {
  echo "ERROR: Frontend check failed"
  exit 1
}

echo "Testing MQTT port..."
if command -v nc >/dev/null 2>&1; then
  nc -z "${MQTT_HOST_IP_VALUE}" "${MQTT_PORT}" || {
    echo "ERROR: MQTT port check failed on ${MQTT_HOST_IP_VALUE}:${MQTT_PORT}"
    exit 1
  }
else
  echo "Skipping MQTT TCP check because nc is not installed."
fi

echo
echo "Deployment finished."
