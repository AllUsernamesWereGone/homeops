#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$HOME/homeops/apps/homeops"

echo "Deploying HomeOps from $APP_DIR"

cd "$APP_DIR"

echo "Pulling latest changes..."
git pull --ff-only

echo "Building and starting containers..."
cd infra/compose
docker compose up -d --build

echo "Removing unused Docker images..."
docker image prune -f

echo "Current containers:"
docker compose ps

echo "Deployment finished."
