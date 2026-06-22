#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$HOME/homeops/apps/homeops"
COMPOSE_DIR="$APP_DIR/infra/compose"

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

echo "Testing backend through frontend proxy..."
HOST_IP_VALUE="$(grep '^HOST_IP=' .env | cut -d '=' -f2)"
curl -fsS "http://${HOST_IP_VALUE}:8080/api/v1/system/info" || {
  echo "ERROR: Backend API check failed"
  exit 1
}

echo
echo "Deployment finished."
