#!/bin/bash
# firebase.sh — Deploy dashboard ke Firebase Hosting
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

# Cek firebase CLI
if ! command -v firebase &> /dev/null; then
  echo -e "${RED}✗ firebase CLI tidak ditemukan${NC}"
  echo "  Install dengan:"
  echo "  npm install -g firebase-tools"
  exit 1
fi

# Login jika belum
echo -e "${YELLOW}1. Login ke Firebase (browser akan terbuka)...${NC}"
firebase login --no-localhost 2>/dev/null || firebase login

# Pilih project
echo ""
echo -e "${YELLOW}2. Pilih Firebase Project:${NC}"
firebase use --list 2>/dev/null || true
echo ""
read -p "Project ID atau nama project: " PROJECT

if [ -n "$PROJECT" ]; then
  echo -e "${GREEN}  Menggunakan project: $PROJECT${NC}"
  firebase use "$PROJECT"
fi

# Build / siap deploy
echo ""
echo -e "${YELLOW}3. Siap deploy...${NC}"
echo "  Folder: $(pwd)"
echo "  File yang akan di-upload:"
ls -1 index.html firebase.json 2>/dev/null | sed 's/^/    /'

# Deploy
echo ""
echo -e "${YELLOW}4. Deploy ke Firebase Hosting...${NC}"
firebase deploy --only hosting

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Deploy selesai${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "  Kunjungi: https://PROJECT_ID.web.app"
echo ""
