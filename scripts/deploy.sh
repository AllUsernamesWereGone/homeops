#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$HOME/homeops/apps/homeops-lab"

echo "Deploying HomeOps from $APP_DIR"

cd "$APP_DIR"

echo "Pulling latest changes..."
git pull --ff-only

echo "Starting containers..."
cd infra
docker compose up -d

echo "Current containers:"
docker compose ps

echo "Deployment finished."