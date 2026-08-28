# syntax=docker/dockerfile:1

# Build the static web playtest bundle (ROM must exist in web/public/rom/).
FROM node:22-alpine AS web-build

WORKDIR /app

COPY web/package.json web/package-lock.json ./
RUN npm ci

COPY web/ ./

# CI copies rom/leo_bridge.gba here before docker build.
RUN test -f public/rom/leo_bridge.gba || (echo "Missing public/rom/leo_bridge.gba — run scripts/sync_rom_to_web.sh" && exit 1)

RUN npm run build

FROM nginx:1.27-alpine AS runtime

COPY deploy/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=web-build /app/dist /usr/share/nginx/html

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD wget -qO- http://127.0.0.1/health || exit 1
