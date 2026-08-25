#!/bin/sh

threshold=${UPDATES_THRESHOLD:-200}
count=$(checkupdates 2>/dev/null | wc -l)

if [ "$count" -ge "$threshold" ]; then
	printf '{"text": "%s", "class": "warning"}\n' "$count"
else
	printf '{"text": "%s"}\n' "$count"
fi
