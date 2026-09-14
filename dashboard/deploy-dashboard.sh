#!/bin/bash
# deploy-dashboard.sh — Build + deploy dashboard ke Firebase Hosting
# Jalankan dari dalam folder dashboard/

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo ""
echo "========================================"
echo "  Firebase Hosting — Deploy Dashboard"
echo "========================================"
echo ""

# ── 1. Cek firebase CLI ─────────────────────────────────────
if ! command -v firebase &> /dev/null; then
  echo -e "${RED}✗ firebase CLI tidak ditemukan${NC}"
  echo "  Install dengan: npm install -g firebase-tools"
  exit 1
fi

# ── 2. Build: inject config ke index.html ───────────────────
CONFIG_FILE="firebase-config.json"

if [ ! -f "$CONFIG_FILE" ]; then
  echo -e "${RED}✗ $CONFIG_FILE tidak ditemukan${NC}"
  echo "  Isi dulu config di firebase-config.json"
  echo "  Lihat contoh di: firebase-config.json.example"
  exit 1
fi

echo -e "${YELLOW}1. Membaca firebase-config.json...${NC}"
CONFIG_JSON=$(cat "$CONFIG_FILE")

# Validasi JSON
if ! echo "$CONFIG_JSON" | python -m json.tool > /dev/null 2>&1; then
  echo -e "${RED}✗ firebase-config.json bukan JSON yang valid${NC}"
  exit 1
fi

# Extract apiKey dan databaseURL
API_KEY=$(echo "$CONFIG_JSON" | python -c "import sys,json; print(json.load(sys.stdin).get('apiKey',''))" 2>/dev/null)
DB_URL=$(echo "$CONFIG_JSON" | python -c "import sys,json; print(json.load(sys.stdin).get('databaseURL',''))" 2>/dev/null)

if [ -z "$API_KEY" ] || [ -z "$DB_URL" ]; then
  echo -e "${RED}✗ apiKey dan/atau databaseURL tidak ada di firebase-config.json${NC}"
  exit 1
fi

echo -e "  apiKey: ${API_KEY:0:20}..."
echo -e "  databaseURL: ${DB_URL:0:40}..."

# Build config JS untuk inline
CONFIG_SCRIPT="<script>\n  window.__FIREBASE_CONFIG__ = ${CONFIG_JSON};\n</script>"

# Inject ke index.html — ganti placeholder
if grep -q "FIREBASE_CONFIG_PLACEHOLDER" index.html; then
  echo -e "${YELLOW}2. Menginject config ke index.html...${NC}"
  sed -i "s|FIREBASE_CONFIG_PLACEHOLDER|${CONFIG_SCRIPT}|g" index.html
  echo -e "  ${GREEN}✓ Config berhasil diinject${NC}"
else
  echo -e "${YELLOW}2. Membuat index.html terpisah (build)...${NC}"
  # Jika tidak pakai placeholder, buat file terpisah
  cp index.html index-built.html
  sed -i "s|</head>|${CONFIG_SCRIPT}\n</head>|g" index-built.html
  mv index-built.html index.html
  echo -e "  ${GREEN}✓ Config berhasil diinject${NC}"
fi

# ── 3. Login ke Firebase ─────────────────────────────────────
echo ""
echo -e "${YELLOW}3. Login ke Firebase...${NC}"
firebase login --no-localhost 2>/dev/null || firebase login

# ── 4. Pilih project ─────────────────────────────────────────
echo ""
echo -e "${YELLOW}4. Daftar project:${NC}"
firebase projects:list 2>/dev/null || true
echo ""
read -p "Project ID (kosongkan untuk skip): " PROJECT

if [ -n "$PROJECT" ]; then
  echo -e "${GREEN}  Menggunakan: $PROJECT${NC}"
  firebase use "$PROJECT"
else
  echo -e "${YELLOW}  Pilih project via: firebase use <project-id>${NC}"
fi

# ── 5. Deploy ────────────────────────────────────────────────
echo ""
echo -e "${YELLOW}5. Deploy ke Firebase Hosting...${NC}"
firebase deploy --only hosting --exclude-ci

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Deploy selesai!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "  Dashboard live di:"
echo "  https://<project-id>.web.app"
echo "  atau"
echo "  https://<project-id>.firebaseapp.com"
echo ""
