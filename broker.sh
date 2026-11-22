#!/bin/sh
set -eu

cd "$(dirname "$0")"
mosquitto="$(brew --prefix)/sbin/mosquitto"
python="${ROBOT_TOOLS:-$HOME/.robot-car-tools}/venv/bin/python"

for port in 1883 8080; do
    if nc -z -w 1 127.0.0.1 "$port" 2>/dev/null; then
        echo "Port $port is already in use. Stop the existing service first." >&2
        exit 1
    fi
done

broker_pid=
dashboard_pid=

cleanup() {
    for pid in "$dashboard_pid" "$broker_pid"; do
        if [ -n "$pid" ]; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    wait
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

"$mosquitto" -c config/mosquitto-lab.conf &
broker_pid=$!

attempts=0
until nc -z -w 1 127.0.0.1 1883 2>/dev/null; do
    if ! kill -0 "$broker_pid" 2>/dev/null || [ "$attempts" -ge 50 ]; then
        echo "Mosquitto failed to start." >&2
        exit 1
    fi
    attempts=$((attempts + 1))
    sleep 0.1
done

"$python" scripts/mqtt_client.py serve --broker 127.0.0.1 &
dashboard_pid=$!

while kill -0 "$broker_pid" 2>/dev/null && kill -0 "$dashboard_pid" 2>/dev/null; do
    sleep 1
done

if ! kill -0 "$broker_pid" 2>/dev/null; then
    echo "Mosquitto stopped." >&2
    exit 1
fi

wait "$dashboard_pid"
