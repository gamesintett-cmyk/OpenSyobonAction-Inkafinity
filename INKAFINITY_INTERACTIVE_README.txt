OpenSyobonAction - Inkafinity Interactive Build
================================================

Servidor local integrado:
  http://127.0.0.1:5755

Endpoints de prueba:

1) Estado del servidor
  http://127.0.0.1:5755/status

2) Spawnear enemigos cerca del jugador
  http://127.0.0.1:5755/spawn?enemy=cat&quantity=1&userName=Fernando
  http://127.0.0.1:5755/spawn?enemy=0&quantity=3&userName={nickname}
  http://127.0.0.1:5755/spawn?id=3&quantity=2&userName={nickname}

Aliases soportados:
  cat / syobon / goomba -> enemigo tipo 0
  jump / jumper         -> enemigo tipo 3
  mushroom / kinoko     -> item tipo 100
  poison                -> item tipo 102
  star / badstar        -> item tipo 110
  Tambien puedes usar enemy=NUMERO o id=NUMERO

3) Matar jugador
  http://127.0.0.1:5755/kill?userName={nickname}

4) Limpiar enemigos activos
  http://127.0.0.1:5755/clear

5) Efectos iniciales
  http://127.0.0.1:5755/effect?type=speed&seconds=5&userName={nickname}
  http://127.0.0.1:5755/effect?type=freeze&seconds=5&userName={nickname}

Notas:
- Acepta GET, POST y OPTIONS.
- Tiene CORS abierto para que pueda llamarlo un overlay/webhook local.
- Los nombres enviados por userName/username/nickname se dibujan encima del enemigo.
- quantity esta limitado a 20 por llamada para evitar saturar el juego.

Ejemplos para Inkafinity/TikFinity:
  http://127.0.0.1:5755/spawn?enemy=cat&quantity=1&userName={nickname}
  http://127.0.0.1:5755/spawn?enemy=cat&quantity={repeatCount}&userName={nickname}
  http://127.0.0.1:5755/kill?userName={nickname}
  http://127.0.0.1:5755/effect?type=freeze&seconds=5&userName={nickname}

Compilar en Linux:
  sudo apt install build-essential libsdl1.2-dev libsdl-gfx1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev
  make

Windows:
  Este repo original usa SDL 1.2. Para Windows se recomienda compilar con MinGW/MSYS2 o adaptar el proyecto a Visual Studio con SDL 1.2.
  Si usas MinGW, recuerda linkear ws2_32 para Winsock si tu toolchain lo requiere.
