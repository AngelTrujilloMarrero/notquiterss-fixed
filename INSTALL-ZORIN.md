# NotQuiteRSS-fixed para Zorin 18 / Ubuntu noble

Fork parcheado: corrige segfault por U+FE0F en nombres de feed (BandaAncha) que
tumbaba QuiteRSS 0.19.4 y NotQuiteRSS original en `QTreeView::indexRowSizeHint`
via `FcCharSetHasChar`.

## 1. Dependencias
```bash
sudo apt update && sudo apt install -y python3 gcc g++ make pkgconf \
  qtbase5-dev qt5-qmake qtbase5-dev-tools qttools5-dev-tools \
  libqt5svg5-dev qt5-image-formats-plugins \
  libsqlite3-dev libxml2-dev git gdb
```

## 2. Preparar deps
```bash
python3 scripts/prepare-miniaudio.py --prefix "$HOME/miniaudio-snapshot"
export MINIAUDIO_INCLUDE_DIR="$HOME/miniaudio-snapshot/include"
python3 scripts/prepare-qtsingleapplication.py --work-dir "$HOME/app-deps-qt5" --qmake /usr/bin/qmake --make make
export QMAKEFEATURES="$HOME/app-deps-qt5/install/features"
```

## 3. Compilar e instalar
```bash
mkdir -p _build && cd _build
qmake ../app.pro CONFIG+=release CONFIG-=debug_and_release PREFIX=/usr
make -j$(nproc)
sudo make install
```

## 4. Migrar feeds de QuiteRSS
- Tus feeds viejos: `~/.local/share/QuiteRss/QuiteRss/feeds.db`
- OPML respaldo incluido: `quiterss-feeds.opml`
- URLs ya corregidas a https: Genbeta, Xataka, BandaAncha, elespanol.
- El feed `quiterss.org/en/rss.xml` está muerto (tag mismatch), bórralo.

O importa `quiterss-feeds.opml` desde la GUI.
