HTTPS placeholder

This folder is intentionally empty of private certificate/key material.

Generate development certificates with:

openssl genrsa -out server.key 2048

openssl req -new -x509 \
-key server.key \
-out server.crt \
-days 365 \
-subj "/CN=esp32.local"

Do not commit real private keys to GitHub.
