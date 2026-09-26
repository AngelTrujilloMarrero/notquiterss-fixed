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

## 5. Si la app se cierra sola al abrir una noticia (SIGSEGV)

El crash típico no era del contenido del feed, sino del **caché de
fontconfig**:

```
# síntoma: todo responde la misma fuente corrupta
fc-match "a"          → OpenDyslexic-Bold.woff: "a"
fc-match "Inter"      → OpenDyslexic-Bold.woff: "Inter"
fc-match "sans-serif" → OpenDyslexic-Bold.woff: "Arimo"
```

El paquete `fonts-opendyslexic` instala copias WOFF en
`/usr/share/fonts/woff/`, y fontconfig 2.15 las devuelve como mejor
conicidencia para *cualquier* consulta, con nombres de familia
inventados. Qt 5.15 recibe un `FcCharSet` basura y hace **SIGSEGV en
`FcCharSetHasChar()`** al dar forma al texto:

```
#0  FcCharSetHasChar (fontconfig)
#3  QFontEngineMulti::stringToCMap
#5  QTextEngine::shapeText
#7  QTextDocument::setHtml
```

### Arreglo

```bash
# 1. reconstruir el caché roto
sudo fc-cache -f -r

# 2. que fontconfig ignore los WOFF (el paquete trae los .otf correctos)
sudo cp local.conf /etc/fonts/local.conf   # ver este repo
sudo fc-cache -f -r
```

Verificado: `fc-match "Inter"` → `Inter-Regular.ttf`,
`fc-match "🧳"` → `NotoSans-Regular.ttf`.

### Protección en la app

`Common::sanitizeForDisplay()` (`src/common/common.cpp`) elimina del
texto mostrado los caracteres sin cobertura de fuente, así que aunque
fontconfig vuelva a fallar no forma shaping roto:

- todo código punto no-BMP (emojis y símbolos astrales),
- categorías `Cf` (formato: ZWJ, marcas bidi, BOM), `Cs` (sustitutos),
  `Co` (uso privado) y `Cn` (sin asignar),
- seletores de variación U+FE00..U+FE0F.

Se aplica en `feedsmodel.cpp`, `newsmodel.cpp`, `articlecontent.cpp`,
`notificationsnewsitem.cpp` y `notificationsfeeditem.cpp`.
Se conservan acentos, eñes, signos de puntuación y símbolos comunes
(`€`, `«»`, `—`, `•`, `…`).
