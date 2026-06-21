# HomeOps

HomeOps is a personal DevOps and home infrastructure project focused on plant monitoring and management.

We want to help you become a helicopter parent for your houseplants.

Many dashboards and home automation platforms allow you to display data from or integrate with various devices. We want you to have the specialized tools and features you need to effectively manage your garden or plants at any scale.

The goal of this repository is to build a lightweight full-stack system that can run on low-power hardware, including Raspberry Pis.

## Features

### Connect and Manage IoT Devices

#### Visualize Parameters

* Air humidity
* Soil moisture
* Temperature
* Light exposure
* Power consumption
* Power creation
* Mining/Folding/Network Monitoring
* Device status (on/off, open/closed)

#### Interact with Devices

* Turn lamps on or off
* Turn ventilation systems on or off
* Turn pumps on or off

### Automation

* Create custom weather conditions
* Automate actions based on weather forecasts
* Define rules and schedules for plant care

### Community Features

* Let friends help control the environment for your plants
* Share your garden's progress and growth with others

## Vision

HomeOps aims to combine home infrastructure management, IoT automation, and plant care into a single platform. Whether you're monitoring a single houseplant or managing an entire greenhouse, HomeOps provides the tools needed to observe, automate, and optimize your growing environment.

---

## Current Status

| Area                    | Status                       |
| ----------------------- | ---------------------------- |
| Backend                 | Spring Boot backend created  |
| Frontend                | Angular frontend created     |
| Local Docker Compose    | Working                      |
| Raspberry Pi Deployment | Working                      |
| Tailscale Access        | Working                      |
| CI/CD                   | Planned                      |
| Monitoring              | Planned                      |
| Database                | Planned                      |
| IoT/Sensor Integration  | Planned                      |

Current deployment target:

```text
Windows development machine
  -> GitHub repository
  -> Raspberry Pi via Tailscale
  -> Docker Compose
  -> Angular frontend
  -> Spring Boot backend
```

---

## Tech Stack

### Backend

* Java 25
* Spring Boot 4.1.x
* Spring Web MVC
* Spring Boot Actuator
* Spring Data JPA
* H2 Database
* Validation
* Lombok
* Maven

### Frontend

* Angular
* TypeScript
* SCSS
* Nginx for production container serving

### Infrastructure

* Docker
* Docker Compose
* Raspberry Pi 4B
* Mosquitto MQTT
* Tailscale
* UFW firewall
* GitHub

### Development Tools

* IntelliJ IDEA Ultimate
* Git Bash on Windows
* Docker Desktop on Windows
* Vim for terminal editing when needed

---

## Repository Structure

```text
homeops/
├── backend/
│   ├── Dockerfile
│   ├── pom.xml
│   └── src/
│
├── frontend/
│   ├── Dockerfile
│   ├── package.json
│   └── src/
│
├── infra/
│   ├── docker-compose.yml
│   ├── mosquitto/
│   │   ├── mosquitto.conf
│   │   ├── data/
│   │   └── log/
│   ├── .env.example
│   └── .env
│
├── scripts/
│   └── deploy.sh
│
├── docs/
│   ├── deployment.md
│   └── mqtt-topics.md
│
├── .gitignore
└── README.md
```

---

## Architecture

The deployed application currently runs as two containers:

```text
                       +----------------+
                       | Angular UI     |
                       +----------------+
                                |
                                | HTTP/REST
                                v
                       +----------------+
                       | Spring Boot    |
                       | Backend        |
                       +----------------+
                          ^          |
               MQTT       |          | MQTT
               Subscribe  |          | Publish
                          |          v
                      +-------------------+
                      | Mosquitto Broker  |
                      +-------------------+
                         ^            ^
                         |            |
                     MQTT|            |MQTT
                         |            |
                         |            v
                 +-----------+       +-----------+
                 | Sensor    |       | Actuator  |
                 | ESP32     |       | Pump/LED  |
                 +-----------+       +-----------+
```

Only the frontend container is exposed to the host.
The backend is reachable inside the Docker network but is not directly exposed as a host port.

### Container Layout

```text
homeops-frontend
  - Serves Angular via Nginx
  - Exposes port 80 inside the container
  - Bound to host port 8080

homeops-backend
  - Runs Spring Boot
  - Listens on port 8080 inside the container
  - Accessed internally by the frontend container
```

---

## Configuration

Runtime configuration is handled through an `.env` file inside the `infra/` directory.

The real `.env` file is ignored by Git.

Example:

```env
HOST_IP=127.0.0.1
```

### Local Windows Development

For local Docker testing on Windows:

```env
HOST_IP=127.0.0.1
```

The app is then available at:

```text
http://localhost:8080
```

### Raspberry Pi Deployment

For deployment on the Raspberry Pi, use the Pi's Tailscale IP:

```env
HOST_IP=100.x.x.x
```

The app is then available at:

```text
http://homeops-pi:8080
```

or:

```text
http://100.x.x.x:8080
```

Do not commit the real `.env` file.

---

## Development

### Prerequisites

Install the following on the development machine:

* Git
* Java 25
* Node.js / npm
* Docker Desktop
* IntelliJ IDEA Ultimate
* Git Bash

The Raspberry Pi should have:

* Docker
* Docker Compose plugin
* Git
* Tailscale
* UFW configured for SSH and application access over Tailscale

---

## Local Development Without Docker

### Backend

From Git Bash:

```bash
cd /d/dev/homeops/backend
./mvnw spring-boot:run
```

The backend should start on:

```text
http://localhost:8080
```

Useful checks:

```bash
curl http://localhost:8080/actuator/health
```

### Frontend

From another Git Bash terminal:

```bash
cd /d/dev/homeops/frontend
npm install
npm start
```

The frontend should start on:

```text
http://localhost:4200
```

If an Angular proxy is configured, frontend requests to `/api` should be forwarded to the backend on port `8080`.

---

## Local Development With Docker Compose

From Git Bash:

```bash
cd /d/dev/homeops/infra
docker compose up -d --build
```

Check running containers:

```bash
docker compose ps
```

Check logs:

```bash
docker compose logs homeops-backend
docker compose logs homeops-frontend
```

Test locally:

```bash
curl http://127.0.0.1:8080
curl http://127.0.0.1:8080/actuator/health
```

Open in browser:

```text
http://localhost:8080
```

Stop the stack:

```bash
docker compose down
```

---

## Deployment

The Raspberry Pi is the current deployment target.

### Deployment Target

```text
Host: homeops-pi
Access: ssh pi@homeops-pi
Network: Tailscale
Application path: ~/homeops/apps/homeops
Compose path: ~/homeops/apps/homeops/infra
```

### Manual Deployment

SSH into the Pi:

```bash
ssh pi@homeops-pi
```

Go to the app directory:

```bash
cd ~/homeops/apps/homeops
```

Pull the latest changes:

```bash
git pull --ff-only
```

Start or rebuild the stack:

```bash
cd infra
docker compose up -d --build
docker compose ps
```

Check logs:

```bash
docker compose logs homeops-backend
docker compose logs homeops-frontend
```

Test from the Pi:

```bash
curl http://$(tailscale ip -4):8080
curl http://$(tailscale ip -4):8080/actuator/health
```

Open from another Tailscale-connected machine:

```text
http://homeops-pi:8080
```

---

## Deployment Script

The repository contains a deployment script:

```text
scripts/deploy.sh
```

Expected usage on the Raspberry Pi:

```bash
cd ~/homeops/apps/homeops
./scripts/deploy.sh
```

The script should:

1. Pull the latest changes from GitHub
2. Rebuild and restart the Docker Compose stack
3. Remove unused Docker images
4. Show the current container status

---

## Usage

At the current stage, the app is mainly a deployment foundation.

### Current Usage

Open the frontend:

```text
http://localhost:8080
```

or on the Raspberry Pi through Tailscale:

```text
http://homeops-pi:8080
```

The frontend is served by Nginx.

Backend health can be checked through the frontend proxy:

```text
http://homeops-pi:8080/actuator/health
```

### Planned Usage

In future versions, the dashboard should display:

* Raspberry Pi server status
* Backend health
* Current application version
* Connected devices
* Antminer status
* Sensor readings
* Deployment status
* Monitoring alerts
* Backup status

---

## Current Docker Compose Setup

The Compose stack currently contains:

```text
homeops-backend
homeops-frontend
homeops-net
```

The frontend exposes the public application port.

The backend is only reachable inside the Docker network.

Expected port mapping on the Raspberry Pi:

```text
100.x.x.x:8080 -> homeops-frontend:80
```

The backend should show only an internal port:

```text
homeops-backend:8080
```

---

## Troubleshooting

### Port 8080 Is Already Allocated

Error example:

```text
Bind for 100.x.x.x:8080 failed: port is already allocated
```

Check which container uses the port:

```bash
docker ps --format "table {{.Names}}\t{{.Image}}\t{{.Ports}}"
```

Check the process:

```bash
sudo ss -ltnp | grep ':8080'
```

If an old test container is using the port, remove it:

```bash
docker stop homeops-web
docker rm homeops-web
```

Restart the stack:

```bash
cd ~/homeops/apps/homeops/infra
docker compose down
docker compose up -d
```

---

### Docker Desktop Pipe Error on Windows

Error example:

```text
open //./pipe/dockerDesktopLinuxEngine: The system cannot find the file specified
```

This usually means Docker Desktop is not running or the Linux engine is unavailable.

Fix:

1. Start Docker Desktop
2. Wait until it is fully running
3. Check:

```bash
docker version
docker info
docker context ls
```

If needed:

```bash
docker context use desktop-linux
```

---

### Wrong HOST_IP

If local Windows Docker tries to bind to the Raspberry Pi's Tailscale IP, startup fails.

For local Windows Docker:

```env
HOST_IP=127.0.0.1
```

For Raspberry Pi deployment:

```env
HOST_IP=100.x.x.x
```

Rule:

```text
HOST_IP must be an IP address owned by the machine running Docker Compose.
```

---

## Roadmap

### Phase 1: Full-Stack Foundation

* [x] Create GitHub repository
* [x] Set up Raspberry Pi access through Tailscale
* [x] Install Docker on Raspberry Pi
* [x] Create Spring Boot backend
* [x] Create Angular frontend
* [x] Add Dockerfiles
* [x] Add Docker Compose
* [x] Deploy full stack to Raspberry Pi

### Phase 2: Basic Dashboard

* [ ] Add backend endpoint: `/api/system/info`
* [ ] Add backend endpoint: `/api/devices`
* [ ] Show backend status in Angular
* [ ] Show app version in Angular
* [ ] Show planned devices in Angular
* [ ] Improve dashboard layout

### Phase 3: CI/CD

* [ ] Add GitHub Actions workflow
* [ ] Build backend in CI
* [ ] Build frontend in CI
* [ ] Run backend tests
* [ ] Run frontend tests
* [ ] Add self-hosted runner on spare laptop
* [ ] Deploy to Raspberry Pi from CI

### Phase 4: Database

* [ ] Add PostgreSQL container
* [ ] Replace H2 for deployed environment
* [ ] Add Flyway migrations
* [ ] Persist devices
* [ ] Add settings table

### Phase 5: Monitoring

* [ ] Add Spring Boot Actuator metrics
* [ ] Add Prometheus
* [ ] Add Grafana
* [ ] Add Node Exporter
* [ ] Add cAdvisor
* [ ] Create HomeOps Grafana dashboard

### Phase 6: Device Integration

* [ ] Add MQTT broker
* [ ] Add ESP32 sensor support
* [ ] Add Pico 2W display integration
* [ ] Add Antminer status monitoring
* [ ] Add e-paper display status output

### Phase 7: Security and Operations

* [ ] Add authentication
* [ ] Add role-based admin access
* [ ] Add backup scripts
* [ ] Add restore procedure
* [ ] Add health checks
* [ ] Add log rotation
* [ ] Add update documentation

### Phase 8: Advanced Infrastructure

* [ ] Add Ansible server setup
* [ ] Add automated provisioning
* [ ] Add staging environment
* [ ] Add Kubernetes/k3s experiment
* [ ] Add remote deployment strategy

---

## Notes

This project intentionally starts simple.

The first goal is not to build a complex application immediately.
The first goal is to create a reliable DevOps foundation:

```text
code
  -> build
  -> containerize
  -> deploy
  -> observe
  -> automate
```
