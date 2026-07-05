#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$HOME/homeops"
COMPOSE_DIR="$APP_DIR/infra/compose"

MQTT_HOST_IP_VALUE="$(grep '^MQTT_HOST_IP=' .env | cut -d '=' -f2 || true)"

NO_CACHE="${NO_CACHE:-false}"

echo "Deploying HomeOps from $APP_DIR"

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
  echo "Create it with: HOST_IP=<pi-tailscale-ip>"
  exit 1
fi

cd "$COMPOSE_DIR"

echo "Validating Docker Compose config..."
docker compose config >/dev/null

echo "Stopping old containers and removing orphans..."
docker compose down --remove-orphans

if [ "$NO_CACHE" = "true" ]; then
  echo "Building containers without cache..."
  docker compose build --no-cache
  echo "Starting containers..."
  docker compose up -d --force-recreate
else
  echo "Building and starting containers..."
  docker compose up -d --build --force-recreate
fi

echo "Removing unused Docker images..."
docker image prune -f

echo "Current containers:"
docker compose ps

echo "Waiting for backend healthcheck..."

for i in {1..30}; do
  BACKEND_HEALTH="$(docker inspect --format='{{.State.Health.Status}}' homeops-backend 2>/dev/null || echo starting)"

  if [ "$BACKEND_HEALTH" = "healthy" ]; then
    echo "Backend is healthy."
    break
  fi

  if [ "$BACKEND_HEALTH" = "unhealthy" ]; then
    echo "ERROR: Backend became unhealthy"
    docker compose logs --tail=20 homeops-backend
    exit 1
  fi

  echo "Backend health: $BACKEND_HEALTH"
  sleep 3
done

BACKEND_HEALTH="$(docker inspect --format='{{.State.Health.Status}}' homeops-backend 2>/dev/null || echo missing)"

if [ "$BACKEND_HEALTH" != "healthy" ]; then
  echo "ERROR: Backend did not become healthy. Final status: $BACKEND_HEALTH"
  docker compose logs --tail=50 homeops-backend
  exit 1
fi


echo "Testing backend through frontend proxy..."
HOST_IP_VALUE="$(grep '^HOST_IP=' .env | cut -d '=' -f2)"
curl -fsS "http://${HOST_IP_VALUE}:8080/api/v1/system/info" || {
  echo "ERROR: Backend API check failed"
  exit 1
}

echo "Testing frontend..."
curl -fsS "http://${HOST_IP_VALUE}:8080/" >/dev/null || {
  echo "ERROR: Frontend check failed"
  exit 1
}


echo "Testing MQTT port..."
if command -v nc >/dev/null 2>&1; then
  nc -z "${HOST_IP_VALUE}" 1883 || {
    echo "ERROR: MQTT port check failed on ${HOST_IP_VALUE}:1883"
    exit 1
  }
else
  echo "Skipping MQTT TCP check because nc is not installed."
fi

echo
echo "Deployment finished."
