# HomeOps Project TODO

This TODO is intentionally small. The MQTT contract details should stay in a separate document.

## 1. Configure GitHub for Working Together

- [x] Add all collaborators to the GitHub repository.
- [x] Agree on branch workflow.
  - Suggested: `dev` as integration branch, feature branches for work, `main` as stable/release branch.
- [x] Protect important branches.
  - Require pull requests before merging into `dev` and `main`.
  - Require CI to pass before merging.
- [x] Keep the existing CI workflow working for backend tests and frontend build.
- [ ] Add issue templates for bugs, features, backend tasks, frontend tasks, firmware tasks, and infrastructure tasks.
- [ ] Add a pull request template with a checklist.
- [ ] Decide who reviews backend, frontend, firmware, and infrastructure changes.

## 2. Agree on the MQTT Contract

- [ ] Create one separate document for the MQTT contract.
  - Suggested file: `docs/mqtt-contract.md`
- [ ] Agree on broker address, port, username/password, and whether TLS is needed later.
- [ ] Agree on topic structure.
- [ ] Agree on device ID naming.
- [ ] Agree on the shared message envelope.
- [ ] Agree on required message types.
  - Telemetry
  - State
  - Command
  - Command result
  - Error
  - Hello / startup message
- [ ] Agree on how dynamic sensor data is represented.
- [ ] Agree on how command success and failure are reported.
- [ ] Agree on telemetry frequency.
- [ ] Agree on reconnect behavior.
- [ ] Keep the contract versioned with `schemaVersion`.

## 3. Talk About the UI

- [ ] Decide the main frontend pages.
  - Dashboard
  - Devices list
  - Device detail
  - Device create/edit
  - Telemetry/status view
  - Command/control view
  - Settings/config page
- [ ] Decide what the dashboard should show first.
  - Online/offline devices
  - Latest sensor values
  - Recent commands
  - Warnings/errors
- [ ] Decide how device controls should look.
  - Buttons
  - Toggles
  - Status badges
  - Confirmation for risky actions
- [ ] Decide whether the first version uses polling or live updates.
  - Suggested: start with polling, add WebSocket/SSE later.
- [ ] Keep UI modular so new device types can get their own pages later.

## 4. Prepare Config Files

- [ ] Prepare Docker Compose config.
  - Suggested file: `infra/compose/docker-compose.yml`
- [ ] Prepare environment example file.
  - Suggested file: `infra/compose/.env.example`
- [ ] Prepare Mosquitto config.
  - Suggested file: `infra/mqtt/mosquitto.conf`
- [ ] Prepare backend MQTT config.
  - Suggested location: `apps/backend/src/main/resources/application.yml`
- [ ] Prepare frontend proxy config for local development.
  - Suggested file: `apps/frontend/proxy.conf.json`
- [ ] Prepare Nginx config for frontend/backend routing.
  - Suggested location: `apps/frontend/nginx/`
- [ ] Prepare deployment script.
  - Suggested file: `scripts/deploy.sh`
- [ ] Prepare smoke test script.
  - Suggested file: `scripts/smoke-test.sh`

## 5. Agree on Routing

- [ ] Backend API routes should be stable.
  - Suggested base: `/api/v1`
- [ ] Device API should use predictable routes.
  - Example: `/api/v1/devices`
  - Example: `/api/v1/devices/{deviceId}`
  - Example: `/api/v1/devices/{deviceId}/commands`
- [ ] Angular routes should match the page structure.
  - Example: `/dashboard`
  - Example: `/devices`
  - Example: `/devices/:deviceId`
  - Example: `/devices/:deviceId/edit`
- [ ] Nginx should route frontend and backend clearly.
  - `/` serves Angular.
  - `/api/` proxies to Spring Boot.
  - `/health` proxies to backend health endpoint.

## 6. Deployment Strategy

- [ ] Decide the deployment target.
  - First target: local Docker on dev PC.
  - Later target: Raspberry Pi 4 or old laptop.
- [ ] Decide how deployment is triggered.
  - Manual script first.
  - GitHub Actions deployment later.
- [ ] Define the deployment flow.
  - Pull latest code.
  - Check required config files.
  - Build containers.
  - Restart stack.
  - Run smoke tests.
- [ ] Decide where persistent data lives.
  - Database files / volume.
  - Mosquitto config/data/logs.
- [ ] Decide how rollback works.
  - Keep previous Git commit available.
  - Keep persistent data separate from containers.
- [ ] Document exact deployment commands.
