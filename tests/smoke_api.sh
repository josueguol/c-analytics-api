#!/bin/sh

set -eu

base_url="${1:-http://localhost:8080}"
anonymous_id="8de3c6d1-05bb-4b74-82bc-518f1f8b0871"
email="smoke-test@example.com"
password="correct-horse-battery"

registration_json="$(curl -fsS -X POST "${base_url}/auth/register" \
    -H 'Content-Type: application/json' \
    --data "{\"email\":\"${email}\",\"password\":\"${password}\",\"display_name\":\"Smoke Test\"}")"
confirmation_token="$(printf '%s' "${registration_json}" | jq -er '.confirmation_token')"

curl -fsS -X POST "${base_url}/auth/confirm" \
    -H 'Content-Type: application/json' \
    --data "{\"token\":\"${confirmation_token}\"}" >/dev/null

login_json="$(curl -fsS -X POST "${base_url}/auth/login" \
    -H 'Content-Type: application/json' \
    --data "{\"email\":\"${email}\",\"password\":\"${password}\"}")"
access_token="$(printf '%s' "${login_json}" | jq -er '.access_token')"

curl -fsS -X PUT "${base_url}/me/favorite-sections/science" \
    -H "Authorization: Bearer ${access_token}" >/dev/null
curl -fsS "${base_url}/me/favorite-sections" \
    -H "Authorization: Bearer ${access_token}" | jq -e '.items == ["science"]' >/dev/null

curl -fsS -X POST "${base_url}/activity" \
    -H 'Content-Type: application/json' \
    --data "{\"anonymous_id\":\"${anonymous_id}\",\"portal_key\":\"smoke\",\"event_type\":\"content_view\",\"content_id\":\"note-smoke\"}" \
    | jq -e '.event_id > 0' >/dev/null

curl -fsS -X PUT "${base_url}/content/note-smoke/like" \
    -H 'Content-Type: application/json' \
    --data "{\"anonymous_id\":\"${anonymous_id}\",\"portal_key\":\"smoke\"}" >/dev/null

curl -fsS -X POST "${base_url}/content/note-smoke/comments" \
    -H 'Content-Type: application/json' \
    --data "{\"anonymous_id\":\"${anonymous_id}\",\"portal_key\":\"smoke\",\"body\":\"Smoke comment\"}" \
    | jq -e '.id != null' >/dev/null

curl -fsS "${base_url}/analytics/top-content?portal_key=smoke&days=7&limit=10" \
    -H "Authorization: Bearer ${access_token}" \
    | jq -e '.items[0].content_id == "note-smoke" and .items[0].views == 1' >/dev/null

curl -fsS -X DELETE "${base_url}/auth/session" \
    -H "Authorization: Bearer ${access_token}" >/dev/null

status="$(curl -sS -o /dev/null -w '%{http_code}' "${base_url}/me/favorite-sections" \
    -H "Authorization: Bearer ${access_token}")"
test "${status}" = "401"

printf '%s\n' "API smoke test passed"
