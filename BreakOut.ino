/**************************************************************************
 *                                  
 *   ==== ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====                              
 *   ==== ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====                                                    
 *   ==== ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====                                 
 *             o                     
 *                                    
 *                                 
 *                                    
 *                            ----        
 *                                    
 *                    <<<<<<<< B R E A K O U T >>>>>>>
 *    <<<<<<<< Copyright Ivan Cerra De Castro - Febrero 2021 >>>>>>>>>>>
 *           <<<<<<<< https://www.ivancerra.com >>>>>>>>>>>
 * 
 *                 Video: https://youtu.be/
 * ************************************************************************
 * Emulacion de BreakOut para Arduino 
 * 
 * Componentes usados y conexiones:
 * 1. Arduino UNO.
 * 2. TFT AZDelivery SPI ST7735 128x160.
 *    Conexiones: 
 *        · LED -> 3.3V
 *        · SCK -> 13
 *        · SDA -> 11
 *        · A0 -> 9
 *        · RESET -> 8
 *        · CS -> 10
 *        · GND -> GND
 *        · VCC -> 5V
 * 3. Buzzer Pasivo. 
 *    Conexiones:
 *        · Puerto -> 6
 * 4. Boton Player con Resistencia 1K en PullDown.
 *    Conexiones:
 *        · Puerto -> 2
 * 6. Potenciometro Player.
 *    Conexiones:
 *        · Puerto -> A0
 **************************************************************************/

// Libreria Grafica
#include <Adafruit_GFX.h>
// HAL de Chip ST7735 TFT Controller    
#include <Adafruit_ST7735.h> 
// Libreria del Puerto Serie de Arduino
#include <SPI.h>

// Puertos para TFT
#define TFT_CS        10
#define TFT_RST        8 
#define TFT_DC         9

// Puerto de Potenciometro
#define POTENTIOMETER A0

// Puerto para Buzzer
#define BUZZER 6

// Puerto para Player Coin
#define PLAYER_COIN 2

// Sonidos
#define SOUND_BALL_WALL 1000
#define SOUND_BALL_PADDLE 1500
#define SOUND_BALL_BRICK 2000

// Colores
#define WHITE 0xFFFF
#define BLACK 0x0000
#define GRAY 0xBDF7
#define GRAY_DARK 0x7BEF
#define RED 0xF800
#define RED_DARK 0x7800
#define CYAN 0x07FF
#define CYAN_DARK 0x03EF
#define YELLOW 0xFFE0
#define YELLOW_DARK 0xDEE0
#define BLUE 0x001F
#define BLUE_DARK 0x000F
#define GREEN 0x07E0
#define GREEN_DARK 0x03E0
#define ORANGE 0xDEE0
#define ORANGE_DARK 0x7BE0
#define PURPLE_DARK 0x780F

// Paleta Colores
#define PALETTE_GRAY 6
#define PALETTE_RED 1
#define PALETTE_CYAN 2
#define PALETTE_YELLOW 3
#define PALETTE_BLUE 4
#define PALETTE_GREEN 5

// Dip Switches de Juego ;)
#define SPEED_INI 25
#define SPEED_RATE 5
#define EXTRA_BALL 500
#define ANTI_LOOP 50

// Matrix De Ladrillos
#define BRICKS_COLS 7
#define BRICKS_ROWS 10

// Estructura para tamaño
typedef struct tagSize
{
  int width;
  int height;
}Size;

// Estructura para coordenadas
typedef struct tagPoint
{
  int x;
  int y;
}Point;

// Estructura para velocidad
typedef struct tagSpeed
{
  int dx;
  int dy;
}Speed;

// Estructura del juego
typedef struct tagGame
{
  // Pantalla
  Size screen;
  Size screenHalf;

  // Clipping
  Point clipping;

  // Pala
  Size paddle;
  Size paddleHalf;
  int paddle8Seg;

  // Arena
  Point arenaPosition;
  Size arenaSize;
  
  // Bola
  int ballSize;
  Point ball;
  Point ballOld;
  Speed ballSpeed;

  // Player
  Point player;
  Point playerOld;

  // Bricks
  Size brick;
  // Ladrillos que forman el nivel
  int8_t bricks[BRICKS_COLS][BRICKS_ROWS];
 
  // Patron de fondo
  int patternSize;
  uint16_t patternColorF;
  uint16_t patternColorB;

  // Scores
  Point lblPlayerScore;
  Point playerScore;
  Point lblPlayerLevel;
  Point playerLevel;
  Point lblPlayerLifes;
  Point playerLifes;
  
  // Score
  uint16_t score;

  // Vidas
  uint8_t lifes;

  // Nivel
  uint8_t level;

  // Estado del juego
  bool playing;

  // Tiempo para que aparezca la pelota
  int timeBall;

  // Tiempo para acelerar juego
  uint16_t delayGame;

  // Golpes en pared antes de volver a inicio para no dejar en bucle infinito
  uint8_t antiLoop;

  // Tiempo para que empice cada bola
  uint16_t counter;
}Game;

// Inicializacion de TFT 1.8 128 x 160 
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Paleta de Colores
uint16_t _palette[9][2] = {{WHITE, BLACK}, {RED, RED_DARK}, {CYAN, CYAN_DARK}, {YELLOW, YELLOW_DARK}, {BLUE, BLUE_DARK}, {GREEN, GREEN_DARK}, {GRAY, GRAY_DARK}, {GRAY, GRAY_DARK}, {ORANGE, ORANGE_DARK}};


// Variables de Juego
Game _game;

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   I N I C I A L I Z A C I O N
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Rutinas de inicializacion
 **************************************************************************/
void setup(void) 
{
  // Inicializamos sensores
  setupSensors();

  // Inicializamos juego
  setupGame();

  // Borramos la pantalla
  tft.fillScreen(BLACK);

  // Pintamos etiquetas marcadores
  drawScoreLabel();
  drawLevelLabel();
  drawLifesLabel();

  // Ponemos marcadores a cero
  setScore(0);
  setLevel(0);
  setLifes(0);

  // Pintamos el borde
  drawBorder();

  // Pintamos el fondo
  loadLevel();
  drawBackGround();
  drawBricksLevel();

  // Empieza en Demo
  initDemo();
}

/**************************************************************************
  Inicializacion de sensores
 **************************************************************************/
void setupSensors()
{
  // Inicializamos ST7735S chip, black tab
  tft.initR(INITR_BLACKTAB); 

  // Entrada para Player 1 Coin
  pinMode(PLAYER_COIN, INPUT);

  // Salida para Buzzer
  pinMode(BUZZER, OUTPUT);     
}

/**************************************************************************
  Inicializacion de variables de juego
 **************************************************************************/
void setupGame()
{
  // Dimension de Pantalla
  _game.screen.width = 128;
  _game.screen.height = 160;
  _game.screenHalf.width = _game.screen.width / 2;
  _game.screenHalf.height = _game.screen.height / 2;

  // Clipping
  _game.clipping.x = 0;
  _game.clipping.y = 0;

  // Datos de la pala
  _game.paddle.width = 16;
  _game.paddle.height = 8;
  _game.paddleHalf.width = _game.paddle.width / 2;
  _game.paddleHalf.height = _game.paddle.height / 2;
  _game.paddle8Seg = _game.paddle.width / 8;

  _game.arenaPosition.x = 8;
  _game.arenaPosition.y = 16;
  _game.arenaSize.width = _game.screen.width - _game.arenaPosition.x * 2;
  _game.arenaSize.height = _game.screen.height - _game.arenaPosition.y;

  // Tamaño de la pelota, cogemos el grosor de la pala
  _game.ballSize = 4;

  _game.ball.x = 60;
  _game.ball.y = 100;
  _game.ballSpeed.dx = 1;
  _game.ballSpeed.dy = 2;

  // Bricks
  _game.brick.width = 16;
  _game.brick.height = 8;
  
  // Posicion de los marcadores
  _game.lblPlayerScore.x = 0;
  _game.lblPlayerScore.y = 0;
  _game.playerScore.x = 2 * 8;
  _game.playerScore.y = 0;
  _game.lblPlayerLevel.x = 9 * 8;
  _game.lblPlayerLevel.y = 0;
  _game.playerLevel.x = 10 * 8;
  _game.playerLevel.y = 0;
  _game.lblPlayerLifes.x = 14 * 8;
  _game.lblPlayerLifes.y = 0;
  _game.playerLifes.x = 15 * 8;
  _game.playerLifes.y = 0;
  
  // Tamaño del patron de fondo
  _game.patternSize = 8;
  
  // Ponemos la posicion de los jugadores
  readPlayerPotentiometer(POTENTIOMETER, &(_game.player));
  _game.player.y = _game.arenaSize.height - _game.paddle.height * 2;
  
  // Iniciamos la generacion de numeros aleatorios
  randomSeed(analogRead(POTENTIOMETER));

  // Contador
  _game.counter = 0;
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E  P R O G R A M A   P R I N C I P A L
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Bucle principal
 **************************************************************************/
void loop() 
{
  if(_game.playing)
  {
    loopPlaying();

    return;
  }

  loopDemo();
}

/**************************************************************************
  Bucle principal en Demo
 **************************************************************************/
void loopDemo() 
{
  if(_game.counter == 0)
  {
    drawText("GAME  OVER", 4 * _game.patternSize, 14 * _game.patternSize, 1, RED);
  }

  if(_game.counter == 100)
  {
    for(int i = 4; i < 12; i++)
    {
      drawPattern(i * _game.patternSize - _game.arenaPosition.x, 14 * _game.patternSize - _game.arenaPosition.y);
    }
  }

  // Comprobamos inicio de partida
  checkCoin();

  // Incrementamos contador
  _game.counter = (_game.counter < 150)?_game.counter + 1:0;

  delay(10);
}

/**************************************************************************
  Bucle principal en Juego
 **************************************************************************/
void loopPlaying() 
{
  // Guardamos las posiciones de los actores, no hay memoria para pagina oculta ;)
  oldPositions();

  // Comprobamos si han echado moneda
  checkCoin();

  // Leemos el player 1
  readPlayerPotentiometer(POTENTIOMETER, &(_game.player));

  // Leemos la pelota
  readBall();

  // Pintamos la pelota
  drawBall(_game.ballOld, _game.ball);
    
  // Pintamos la nave;
  drawShip(_game.playerOld, _game.player);

  // Velocidad de juego proporcional a toques de jugador
  delay(_game.delayGame / SPEED_RATE);
}

/**************************************************************************
  Guardamos las posiciones antiguas
 **************************************************************************/
void oldPositions()
{
  _game.playerOld.x = _game.player.x;
  _game.playerOld.y = _game.player.y;

  _game.ballOld.x = _game.ball.x;
  _game.ballOld.y = _game.ball.y;
}

/**************************************************************************
  Comprobacion de monedas
 **************************************************************************/
void checkCoin()
{
  // Si estamos jugando no comenzamos nuevas partidas
  if(_game.playing)
  {
    return;
  }

  // Leemos Boton de Player 1
  int valuePlayer1Coin = digitalRead(PLAYER_COIN); 
  
  // Si hemos pulsado boton de Player 1, comenzamos partida
  if (valuePlayer1Coin == HIGH) 
  {
    initPlay();
    
    return;
  }
}

/**************************************************************************
  Inicio de Demo
 **************************************************************************/
void initDemo()
{
  // Pintamos el borde
  drawBorder();

  // Indicamos que no estamos jugando
  _game.playing = false;

  _game.counter = 0;
}

/**************************************************************************
  Inicio de Partida
 **************************************************************************/
void initPlay()
{
  // Sonido de partida
  soundPlay();

  // inicializa score;
  setScore(0);

  // inicializa vidas;
  setLifes(3);

  // Inicializa el nivel
  setLevel(0);

  // Inicializa el nivel
  initLevel();

  // Comienza el juego  
  _game.playing = true;

}

/**************************************************************************
  Inicio de Nivel
 **************************************************************************/
void initLevel()
{
  // Load Level
  loadLevel();

  // Pintamos el fondo
  drawBackGround();

  // Pintamos Level
  drawBricksLevel();

  // Leemos el player 1
  readPlayerPotentiometer(POTENTIOMETER, &(_game.player));

  int xf = _game.player.x;

  // Ponemos la nave fuera
  _game.player.x = -_game.paddle.width;

  // Esperamos
  delay(100);

  // Movemos la nave
  for(int x = _game.player.x; x < xf; x++)
  {
    oldPositions();

    _game.player.x = x;

    // Para resolver el clipping fuera del marco :(
    if(_game.player.x <= 0)
    {
      drawPattern(-_game.patternSize, _game.arenaSize.height - _game.patternSize * 2);
    }   

    drawShip(_game.playerOld, _game.player);

    // Sonido de entrada
    tone(BUZZER, 2000 + x * 50 , 5);
    delay(10);
  }

  // Pintamos el trozo que nos comimos :) 
  drawBorder2(0, _game.screen.height - 16);

  readyPlayer();
}

/**************************************************************************
  Perdemos la bola
 **************************************************************************/
void lostBall()
{
  for(int freq = 2000; freq > 1000; freq -= 10)
  {
    tone(BUZZER, freq, 2);

    delay(1);
  }

  delay(500);

  // Quitamos una vida
  setLifes(_game.lifes - 1);

  // No tenemos mas vidas
  if(_game.lifes == 0)
  {
    // Sacamos la nave de la pantalla
    _game.player.x = _game.screen.width + _game.paddle.width * 2;
    drawShip(_game.playerOld, _game.player);
    oldPositions();

    // Pintamos el trozo que nos comimos :) 
    drawBorder2(_game.screen.width - _game.patternSize, _game.screen.height - _game.patternSize * 2);

    // Nos vamos a la Demo
    initDemo();

    return;
  }

  // Si no ha terminado la partida, avisamos al player
  readyPlayer();
}

/**************************************************************************
  Avisamos al player que este listo
 **************************************************************************/
void readyPlayer()
{
  drawText("READY!", 6 * _game.patternSize, 14 * _game.patternSize, 1, WHITE);

  delay(2000);

  for(int i = 6; i < 12; i++)
  {
    drawPattern(i * _game.patternSize - _game.arenaPosition.x, 14 * _game.patternSize - _game.arenaPosition.y);
  }

  // Pintamos el borde
  drawBorder();

  delay(1000);

  // Colocamos la pelota
  initBall();
}

/**************************************************************************
  Comprobamos si hemos completado el nivel y nos vamos a siguiente Nivel
 **************************************************************************/
void checkNextLevel()
{
  for(int col = 0; col < BRICKS_COLS; col++)
  {
    for(int row = 0; row < BRICKS_ROWS; row++)
    {
      uint8_t brick = _game.bricks[col][row];

      if(brick > 0 && brick < 8)
      {
        return;
      }
    } 
  }

  // Si hemos llegado aqui, hemos terminado el nivel :)
  delay(200);

  drawShip(_game.playerOld, _game.player);

  // Quitamos trozo de salida
  drawPattern(_game.arenaSize.width, _game.arenaSize.height - _game.patternSize * 2);

  // Movemos la nave
  for(int x = _game.player.x; x < _game.screen.width; x++)
  {
    oldPositions();

    _game.player.x = x;

    drawShip(_game.playerOld, _game.player);

    // Sonido de salida
    tone(BUZZER, 4000 - x * 50 , 15);
  }

  // Esperamos al cierre
  delay(1000);

  // Pintamos el trozo que nos comimos :) 
  drawBorder2(_game.screen.width - _game.patternSize, _game.screen.height - _game.patternSize * 2);

  // Esperamos
  delay(2000);

  // Pasamos de nivel
  setLevel(_game.level + 1);

  initLevel();
}

/**************************************************************************
  Fin de Juego
 **************************************************************************/
void endGame()
{
  // Fin del partido, dejamos en demo
  initDemo();
}

/**************************************************************************
  Incrementamos velocidad de Juego
 **************************************************************************/
void incGameSpeed()
{
  // Vamos incrementando la velocidad
  _game.delayGame = (_game.delayGame > 5 * SPEED_RATE)?_game.delayGame - 1:_game.delayGame;
}

/**************************************************************************
  Comprobamos no estar en bucle
 **************************************************************************/
void checkAntiLoop()
{
  _game.antiLoop--;

  // Hemos entrado en bucle, volvemos a inicio
  if(_game.antiLoop == 0)
  {
    removeBall(_game.ballOld.x, _game.ballOld.y);

    initBall();
  }
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   L E C T U R A   D A T O S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Leemos la Pelota
 **************************************************************************/
void readBall()
{
  if(_game.counter > 0)
  {
    // Decrementamos contador
    _game.counter--;

    return;
  }

  _game.ball.x += _game.ballSpeed.dx;
  _game.ball.y += _game.ballSpeed.dy;

  // Colision con Muro Superior
  if(_game.ball.y <= 0 && _game.ballSpeed.dy < 0)
  {
    _game.ball.y = 0;
    _game.ballSpeed.dy = -_game.ballSpeed.dy;

    soundBall(SOUND_BALL_WALL);

    checkAntiLoop();
  }
  // Colision con Muro Izquierdo
  if(_game.ball.x <= 0 && _game.ballSpeed.dx < 0)
  {
    _game.ball.x = 0;
    _game.ballSpeed.dx = -_game.ballSpeed.dx;

    soundBall(SOUND_BALL_WALL);

    checkAntiLoop();
  }
  // Colision con Muro Derecho
  if(_game.ball.x >= _game.arenaSize.width - _game.ballSize - 1 && _game.ballSpeed.dx > 0)
  {
    _game.ball.x = _game.arenaSize.width - _game.ballSize - 1;
    _game.ballSpeed.dx = -_game.ballSpeed.dx;

    soundBall(SOUND_BALL_WALL);

    checkAntiLoop();
  }

  // Colision con Pala
  checkBallPaddle(_game.player, _game.ball);

  // Colision con Ladrillos
  checkBallBricks(_game.ball);

  // Colision con Parte Inferior
  if(_game.ball.y >= _game.arenaSize.height + _game.ballSize && _game.ballSpeed.dy > 0)
  {
    // *******************************
    // *** Para capturas y pruebas ***
    // *******************************
    //_game.ballSpeed.dy = -_game.ballSpeed.dy;
    //return;

    // Perdemos la bola
    lostBall();
  }
}

/**************************************************************************
  Comprobamos colision Bola - Ladrillos
 **************************************************************************/
void checkBallBricks(Point ball)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  Point cb, ib;
  Size cl;

  // Coordenadas bola
  cb.x = ball.x + _game.ballSize / 2;
  cb.y = ball.y + _game.ballSize / 2;

  // Calculamos el indice de posible ladrillo
  ib.x = cb.x / _game.brick.width;
  ib.y = cb.y / _game.brick.height;

  bool collission;

  // Vamos hacia arriba, miramos en el bloque de arriba
  if(_game.ballSpeed.dy < 0)
  {
    collission = checkBallBrick(cb, ib.x, ib.y - 1);

    if(collission)
    {
      _game.ball.y -= _game.ballSpeed.dy;

      _game.ballSpeed.dy = -_game.ballSpeed.dy;

      removeBrick(ib.x, ib.y - 1);

      return;
    }
  }
  // Vamos hacia abajo, miramos en el bloque de abajo
  if(_game.ballSpeed.dy > 0)
  {
    collission = checkBallBrick(cb, ib.x, ib.y + 1);

    if(collission)
    {
      _game.ball.y -= _game.ballSpeed.dy;

      _game.ballSpeed.dy = -_game.ballSpeed.dy;

      removeBrick(ib.x, ib.y + 1);

      return;
    }
  }
  // Vamos hacia derecha, miramos en el bloque de derecha
  if(_game.ballSpeed.dx > 0)
  {
    collission = checkBallBrick(cb, ib.x + 1, ib.y);

    if(collission)
    {
      _game.ball.x -= _game.ballSpeed.dx;

      _game.ballSpeed.dx = -_game.ballSpeed.dx;

      removeBrick(ib.x + 1, ib.y);

      return;
    }
  }
  // Vamos hacia izquierda, miramos en el bloque de izquierda
  if(_game.ballSpeed.dx < 0)
  {
    collission = checkBallBrick(cb, ib.x - 1, ib.y);

    if(collission)
    {
      _game.ball.x -= _game.ballSpeed.dx;

      _game.ballSpeed.dx = -_game.ballSpeed.dx;

      removeBrick(ib.x - 1, ib.y);

      return;
    }
  }

  collission = checkBallBrick(cb, ib.x, ib.y);
  // Comprobamos la celda en la que estamos
  if(collission)
  {
    _game.ball.x -= _game.ballSpeed.dx;
    _game.ball.y -= _game.ballSpeed.dy;
 
    _game.ballSpeed.dy = -_game.ballSpeed.dy;
 
    removeBrick(ib.x, ib.y);

    return;
  }
}

/**************************************************************************
  Comprobamos colision Bola - Ladrillo
 **************************************************************************/
bool checkBallBrick(Point cb, int colBrick, int rowBrick)
{
  // No estamos en la zona de ladrillos
  if(colBrick >= BRICKS_COLS || rowBrick >= BRICKS_ROWS)
  {
    return false;
  }

  int8_t brick = _game.bricks[colBrick][rowBrick];

  // No hay ladrillo, nos vamos
  if(brick <= 0)
  {
    return false;
  }

  // Hay ladrillo vamos a comprobar las coordenadas, si hay colision
  Point cBrick;

  cBrick.x = colBrick * _game.brick.width + _game.brick.width / 2;
  cBrick.y = rowBrick * _game.brick.height + _game.brick.height / 2;

  Size cl;
  cl.width = _game.ballSize / 2 + _game.brick.width / 2;
  cl.height = _game.ballSize / 2 + _game.brick.height / 2;

  bool collissionX = abs(cBrick.x - cb.x) < cl.width;
  bool collissionY = abs(cBrick.y - cb.y) < cl.height;

  if(collissionX && collissionY)
  {
    return true;
  }

  return false;
}

/**************************************************************************
  Quitamos ladrillo
 **************************************************************************/
void removeBrick(int colBrick, int rowBrick)
{
  Point pos;
  int res = 0;

  // Sonido de colision con ladrillo
  soundBall(SOUND_BALL_BRICK);

  // Aumentamos velocidad de juego
  incGameSpeed();

  if(_game.bricks[colBrick][rowBrick] == 8)
  {
    res = 8;
  }
  else if(_game.bricks[colBrick][rowBrick] == 7)
  {
    res = 6;
  }
 
  _game.bricks[colBrick][rowBrick] = res;
  
  pos.x = colBrick * _game.brick.width;
  pos.y = rowBrick * _game.brick.height;

  // Ladrillo fuerte
  if(res == 8)
  {
    drawBrickStrong(pos.x, pos.y, res);

    return;
  }
  // Cambiamos a ladrillo debil porque era ladrillo fuerte
  else if(res == 6)
  {
    // Sumamos 10 a puntuacion
    setScore(_game.score + 10);

    drawBrick(pos.x, pos.y, res);

    return;
  }

  // Borramos con el patron, ladrillo debil
  // Sumamos 5 a puntuacion
  setScore(_game.score + 5);  

  drawPattern(pos.x, pos.y);
  drawPattern(pos.x + _game.brick.width / 2, pos.y);

  // Comprobamos si hemos completado el nivel
  checkNextLevel();
}

/**************************************************************************
  Comprobamos colision Bola - Pala
 **************************************************************************/
void checkBallPaddle(Point player, Point ball)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  Point cp, cb;
  Size cl;

  cp.x = player.x + _game.paddleHalf.width;
  cp.y = player.y + _game.paddleHalf.height;
  cb.x = ball.x + _game.ballSize / 2;
  cb.y = ball.y + _game.ballSize / 2;
  cl.width = _game.ballSize / 2 + _game.paddleHalf.width;
  cl.height = _game.ballSize / 2 + _game.paddleHalf.height;

  bool collissionX = abs(cp.x - cb.x) < cl.width;
  bool collissionY = abs(cp.y - cb.y) < cl.height;
  
  // Colision con Player 
  if(collissionX && collissionY && _game.ballSpeed.dy > 0)
  {
    // Incrementamos velocidad de juego
    incGameSpeed();

    // Reseteamos el AntiLoop
    _game.antiLoop = ANTI_LOOP;

    _game.ball.x -= _game.ballSpeed.dx;
    _game.ball.y -= _game.ballSpeed.dy;

    // Rebota de sentido
    _game.ballSpeed.dy = - _game.ballSpeed.dy;

    int difX = cp.x - cb.x;
   
    // 8 Segmentos de Pala
    // 2 1 0 0 0 0 1 2 
    // Para que haya 0 0
    if(difX > 0)
    {
      difX--;
    }

    // miramos el segmento para calcular como debe tener el angulo de salida
    difX = difX / _game.paddle8Seg;

    // Para que haya 2 1   1 2
    if(difX > 0)
    {
      difX--;  
    }
    if(difX < 0)
    {
      difX++;
    }
    
    // Segmento golpeado
    // No permitimos angulos mayores de 45º
    int segment = -max(min(difX, 3), -3);
   
    //showLog(segment, 0, 0, 0);

    // Rebote normal si el segmento es el 0
    if(difX != 0)
    {
      _game.ballSpeed.dx = segment;
    }

    soundBall(SOUND_BALL_PADDLE);
  }
}

/**************************************************************************
  Inicio de la Pelota
 **************************************************************************/
void initBall()
{
  _game.antiLoop = ANTI_LOOP;
  _game.counter = 50;
  _game.delayGame = SPEED_INI * 5;
  _game.ball.x = 48;
  _game.ball.y = 96;
  _game.ballSpeed.dx = 1;
  _game.ballSpeed.dy = 2;
}

/**************************************************************************
  Leemos la X del Player 1
 **************************************************************************/
void readPlayerPotentiometer(uint8_t port, Point *pos)
{
  int value = analogRead(port);
  
  pos->x = min(max(map(value, 0, 1023, -16, _game.arenaSize.width), 0), _game.arenaSize.width -_game.paddle.width - 1);
}

/**************************************************************************
  Mostramos log en la primera linea
 **************************************************************************/
void showLog(int value1, int value2, int value3, int value4)
{
  char buf[30];

  fillRect(0, 0, _game.screen.width, 8, BLUE);

  sprintf(buf, "%d %d %d %d", value1, value2, value3, value4);
  drawText(buf,0,0,1,WHITE);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   D E F I N I C I O N   D E   N I V E L E S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Level 1
 **************************************************************************/
void loadLevel1()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 7},
    {1, 1, 1, 1, 1, 1, 1},
    {4, 4, 4, 4, 4, 4, 4},
    {3, 3, 3, 3, 3, 3, 3},
    {5, 5, 5, 5, 5, 5, 5},
    {2, 2, 2, 2, 2, 2, 2},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = BLUE_DARK;
}

/**************************************************************************
  Level 2
 **************************************************************************/
void loadLevel2()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0},
    {1, 2, 0, 0, 0, 0, 0},
    {1, 2, 4, 0, 0, 0, 0},
    {1, 2, 4, 5, 0, 0, 0},
    {1, 2, 4, 5, 3, 0, 0},
    {1, 2, 4, 5, 3, 1, 0},
    {7, 7, 7, 7, 7, 7, 5},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = GREEN_DARK;
}

/**************************************************************************
  Level 3
 **************************************************************************/
void loadLevel3()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {5, 5, 5, 5, 5, 5, 5},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 7, 3, 3, 3, 7},
    {0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0},
    {7, 3, 3, 3, 7, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {4, 4, 4, 4, 4, 4, 4}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = PURPLE_DARK;
}

/**************************************************************************
  Level 4
 **************************************************************************/
void loadLevel4()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 7, 4, 0, 4, 7, 0},
    {0, 4, 1, 0, 7, 5, 0},
    {0, 1, 7, 0, 5, 3, 0},
    {0, 7, 2, 0, 3, 7, 0},
    {0, 2, 5, 0, 7, 2, 0},
    {0, 5, 7, 0, 2, 1, 0},
    {0, 7, 3, 0, 1, 7, 0},
    {0, 3, 4, 0, 7, 4, 0},
    {0, 4, 7, 0, 4, 5, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = RED_DARK;
}

/**************************************************************************
  Level 5
 **************************************************************************/
void loadLevel5()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 3, 0, 0, 0, 3, 0},
    {0, 0, 3, 0, 3, 0, 0},
    {0, 0, 7, 7, 7, 0, 0},
    {0, 7, 6, 6, 6, 7, 0},
    {7, 6, 1, 6, 1, 6, 7},
    {6, 7, 6, 6, 6, 7, 6},
    {7, 0, 6, 7, 6, 0, 7},
    {0, 0, 6, 0, 6, 0, 0},
    {0, 0, 7, 0, 7, 0, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = CYAN_DARK;
}

/**************************************************************************
  Level 6
 **************************************************************************/
void loadLevel6()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {1, 0, 3, 0, 3, 0, 1},
    {1, 7, 3, 7, 3, 7, 1},
    {1, 0, 3, 0, 3, 0, 1},
    {1, 0, 3, 0, 3, 0, 1},
    {1, 0, 3, 0, 3, 0, 1},
    {1, 0, 3, 0, 3, 0, 1},
    {1, 0, 3, 0, 3, 0, 1},
    {1, 0, 3, 0, 3, 0, 1}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = BLUE_DARK;
}

/**************************************************************************
  Level 7
 **************************************************************************/
void loadLevel7()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 2, 7, 3, 0, 0},
    {0, 2, 5, 3, 4, 1, 0},
    {2, 5, 3, 4, 1, 6, 2},
    {5, 3, 4, 1, 6, 2, 5},
    {3, 4, 1, 6, 2, 5, 3},
    {4, 1, 6, 2, 5, 3, 4},
    {0, 6, 2, 5, 3, 4, 0},
    {0, 0, 5, 7, 4, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLUE_DARK;
  _game.patternColorF = CYAN_DARK;
}

/**************************************************************************
  Level 8
 **************************************************************************/
void loadLevel8()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 3, 0, 0, 0, 3, 0},
    {0, 7, 3, 0, 3, 7, 0},
    {0, 0, 0, 6, 0, 0, 0},
    {0, 0, 7, 7, 7, 0, 0},
    {0, 0, 0, 1, 0, 0, 0},
    {0, 0, 7, 2, 7, 0, 0},
    {0, 0, 0, 5, 0, 0, 0},
    {0, 7, 3, 4, 3, 7, 0},
    {0, 3, 0, 3, 0, 3, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = RED_DARK;
}

/**************************************************************************
  Level 9
 **************************************************************************/
void loadLevel9()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {7, 0, 0, 0, 0, 0, 7},
    {7, 5, 0, 0, 0, 5, 7},
    {7, 4, 7, 0, 7, 4, 7},
    {7, 7, 7, 0, 7, 7, 7},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 4, 3, 0, 0},
    {0, 0, 1, 5, 3, 0, 0},
    {0, 0, 1, 2, 3, 0, 0},
    {0, 0, 1, 4, 3, 0, 0}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = GREEN_DARK;
}

/**************************************************************************
  Level 10
 **************************************************************************/
void loadLevel10()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 7, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 7, 0, 0, 4, 0, 0},
    {0, 7, 0, 4, 2, 4, 0},
    {0, 7, 4, 2, 7, 2, 4},
    {0, 7, 0, 4, 2, 4, 0},
    {0, 7, 0, 0, 4, 0, 0},
    {0, 7, 0, 0, 0, 0, 0},
    {0, 7, 0, 0, 0, 0, 0},
    {0, 7, 7, 7, 7, 7, 7}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = CYAN_DARK;
  _game.patternColorF = BLACK;
}

/**************************************************************************
  Level 11
 **************************************************************************/
void loadLevel11()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 7},
    {7, 0, 0, 0, 0, 0, 7},
    {7, 0, 7, 3, 7, 0, 7},
    {7, 0, 7, 0, 7, 0, 7},
    {7, 0, 3, 2, 3, 0, 7},
    {7, 0, 7, 0, 7, 0, 7},
    {7, 0, 7, 3, 7, 0, 7},
    {7, 0, 0, 0, 0, 0, 7},
    {7, 7, 7, 7, 7, 7, 7}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = GREEN_DARK;
}

/**************************************************************************
  Level 12
 **************************************************************************/
void loadLevel12()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 7},
    {0, 0, 0, 7, 0, 0, 0},
    {0, 7, 0, 7, 0, 7, 0},
    {0, 7, 0, 7, 0, 7, 0},
    {0, 7, 2, 7, 2, 7, 5},
    {0, 7, 0, 7, 0, 7, 0},
    {0, 7, 0, 7, 0, 7, 0},
    {0, 7, 0, 0, 0, 7, 0},
    {0, 7, 7, 7, 7, 7, 7}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = PURPLE_DARK;
}

/**************************************************************************
  Level 13
 **************************************************************************/
void loadLevel13()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {1, 0, 3, 3, 3, 0, 1},
    {3, 0, 5, 5, 5, 0, 3},
    {5, 0, 4, 4, 4, 0, 5},
    {4, 0, 2, 2, 2, 0, 4},
    {2, 0, 7, 7, 7, 0, 2},
    {7, 0, 1, 1, 1, 0, 7},
    {3, 0, 5, 5, 5, 0, 3},
    {1, 0, 3, 3, 3, 0, 1}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = RED_DARK;
  _game.patternColorF = BLACK;
}

/**************************************************************************
  Level 14
 **************************************************************************/
void loadLevel14()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {1, 7, 7, 7, 7, 7, 1},
    {7, 0, 0, 0, 0, 0, 7},
    {4, 4, 4, 4, 4, 4, 4},
    {0, 0, 0, 0, 0, 0, 0},
    {3, 7, 7, 7, 7, 7, 3},
    {7, 0, 0, 0, 0, 0, 7},
    {4, 4, 4, 4, 4, 4, 4},
    {0, 0, 0, 0, 0, 0, 0},
    {7, 2, 2, 7, 2, 2, 7}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = CYAN_DARK;
  _game.patternColorF = BLACK;
}

/**************************************************************************
  Level 15
 **************************************************************************/
void loadLevel15()
{
  int8_t bricksLevel [BRICKS_ROWS][BRICKS_COLS] = {
    {0, 0, 0, 0, 0, 0, 0},
    {7, 7, 2, 2, 2, 7, 7},
    {7, 3, 7, 2, 7, 5, 7},
    {7, 3, 3, 7, 5, 5, 7},
    {7, 3, 3, 7, 5, 5, 7},
    {7, 3, 3, 7, 5, 5, 7},
    {7, 3, 3, 7, 5, 5, 7},
    {7, 3, 3, 7, 5, 5, 7},
    {2, 7, 3, 7, 5, 7, 2},
    {2, 2, 7, 7, 7, 2, 2}
  };

  loadBricksLevel(bricksLevel);

  _game.patternColorB = BLACK;
  _game.patternColorF = BLUE_DARK;
}

// Definimos vector de funciones de carga de niveles
void (*pLoadLevelFunction[15])() = {loadLevel1, loadLevel2, loadLevel3, loadLevel4, loadLevel5, loadLevel6, loadLevel7, loadLevel8, loadLevel9, loadLevel10,
                                    loadLevel11, loadLevel12, loadLevel13, loadLevel14, loadLevel15};

/**************************************************************************
  Carga los ladrillos del nivel
 **************************************************************************/
void loadBricksLevel(int8_t bricksLevel[BRICKS_ROWS][BRICKS_COLS])
{
  for(int i = 0; i < BRICKS_ROWS; i++)
  {
    for(int j = 0; j < BRICKS_COLS; j++)
    {
      _game.bricks[j][i] = bricksLevel[i][j]; 
    }
  }
}

/**************************************************************************
  Carga el nivel
 **************************************************************************/
void loadLevel()
{
  // Cargamos el nivel
  pLoadLevelFunction[_game.level]();
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   D I B U J O   A C T O R E S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos Patron 1
 **************************************************************************/
void drawPattern1(int px, int py)
{
  int x = px;
  int y = py;

  drawLineHWithClipping(x, y, 8, _game.patternColorB);
  drawLineHWithClipping(x, y + 7, 8, _game.patternColorB);
  drawLineVWithClipping(x, y, 8, _game.patternColorB);
  drawLineVWithClipping(x + 7, y, 8, _game.patternColorB);
  
  drawLineHWithClipping(x + 1, y + 1, 6, _game.patternColorF);
  drawLineHWithClipping(x + 1, y + 6, 6, _game.patternColorF);
  drawLineVWithClipping(x + 1, y + 1, 6, _game.patternColorF);
  drawLineVWithClipping(x + 6, y + 1, 6, _game.patternColorF);

  drawLineHWithClipping(x + 2, y + 2, 4, _game.patternColorB);
  drawLineHWithClipping(x + 2, y + 5, 4, _game.patternColorB);
  drawLineVWithClipping(x + 2, y + 2, 4, _game.patternColorB);
  drawLineVWithClipping(x + 5, y + 2, 4, _game.patternColorB);

  drawLineHWithClipping(x + 3, y + 3, 2, _game.patternColorF);
  drawLineHWithClipping(x + 3, y + 4, 2, _game.patternColorF);
}

/**************************************************************************
  Pintamos Patron 2
 **************************************************************************/
void drawPattern2(int px, int py)
{
  int x = px;
  int y = py;

  drawLineVWithClipping(x, y, 8, _game.patternColorF);
  drawLineHWithClipping(x, y + 7, 8, _game.patternColorF);
  drawLineHWithClipping(x + 2, y + 1, 5, _game.patternColorF);
  drawLineVWithClipping(x + 6, y + 2, 4, _game.patternColorF);
  drawLineVWithClipping(x + 3, y + 3, 2, _game.patternColorF);
  drawLineHWithClipping(x + 4, y + 4, 1, _game.patternColorF);

  drawLineHWithClipping(x + 1, y, 7, _game.patternColorB);
  drawLineVWithClipping(x + 1, y + 1, 6, _game.patternColorB);
  drawLineVWithClipping(x + 7, y + 1, 6, _game.patternColorB);
  drawLineHWithClipping(x + 2, y + 6, 5, _game.patternColorB);

  drawLineHWithClipping(x + 2, y + 2, 4, _game.patternColorB);
  drawLineHWithClipping(x + 2, y + 5, 4, _game.patternColorB);
  drawLineVWithClipping(x + 2, y + 3, 2, _game.patternColorB);
  drawLineVWithClipping(x + 5, y + 3, 2, _game.patternColorB);
  
  drawLineHWithClipping(x + 4, y + 3, 1, _game.patternColorB);
}

/**************************************************************************
  Pintamos Patron 3
 **************************************************************************/
void drawPattern3(int px, int py)
{
  int x = px;
  int y = py;

  fillRect(x, y, 4, 4, _game.patternColorF);
  fillRect(x + 4, y, 4, 4, _game.patternColorB);
  fillRect(x, y + 4, 4, 4, _game.patternColorB);
  fillRect(x + 4, y + 4, 4, 4, _game.patternColorF);
}

/**************************************************************************
  Pintamos Patron 4
 **************************************************************************/
void drawPattern4(int px, int py)
{
  int x = px;
  int y = py;

  fillRect(x, y + 1, 2, 7, _game.patternColorB);
  fillRect(x + 3, y, 2, 4, _game.patternColorB);
  fillRect(x + 3, y + 5, 2, 3, _game.patternColorB);
  fillRect(x + 6, y, 2, 7, _game.patternColorB);

  drawLineHWithClipping(x, y, 3, _game.patternColorF);
  drawLineHWithClipping(x + 5, y + 7, 3, _game.patternColorF);
  drawLineHWithClipping(x + 3, y + 4, 2, _game.patternColorF);

  drawLineVWithClipping(x + 2, y + 1, 7, _game.patternColorF);
  drawLineVWithClipping(x + 5, y, 7, _game.patternColorF);
}

/**************************************************************************
  Pintamos Patron 5
 **************************************************************************/
void drawPattern5(int px, int py)
{
  int x = px;
  int y = py;

  fillRect(x, y, 2, 4, _game.patternColorB);
  fillRect(x + 6, y, 2, 4, _game.patternColorB);
  fillRect(x + 2, y + 2, 4, 4, _game.patternColorB);
  fillRect(x, y + 6, 8, 2, _game.patternColorB);

  fillRect(x + 2, y, 4, 2, _game.patternColorF);
  fillRect(x, y + 4, 2, 2, _game.patternColorF);
  fillRect(x + 6, y + 4, 2, 2, _game.patternColorF);
}

/**************************************************************************
  Pintamos Patron 6
 **************************************************************************/
void drawPattern6(int px, int py)
{
  int x = px;
  int y = py;

  fillRect(x, y, 2, 2, _game.patternColorB);
  fillRect(x + 6, y, 2, 2, _game.patternColorB);
  fillRect(x, y + 6, 2, 2, _game.patternColorB);
  fillRect(x + 6, y + 6, 2, 2, _game.patternColorB);
  fillRect(x + 2, y + 2, 4, 4, _game.patternColorB);
  drawLineHWithClipping(x + 2, y, 4, _game.patternColorB);
  drawLineHWithClipping(x + 2, y + 7, 4, _game.patternColorB);
  drawLineVWithClipping(x, y + 2, 4, _game.patternColorB);
  drawLineVWithClipping(x + 7, y + 2, 4, _game.patternColorB);
  
  drawLineHWithClipping(x + 2, y + 1, 4, _game.patternColorF);
  drawLineHWithClipping(x + 2, y + 6, 4, _game.patternColorF);
  drawLineVWithClipping(x + 1, y + 2, 4, _game.patternColorF);
  drawLineVWithClipping(x + 6, y + 2, 4, _game.patternColorF);
}

// Definimos vector de funciones de pintado de patrones
void (*pDrawPatternFunction[6]) (int x, int y) = {drawPattern1, drawPattern2, drawPattern3, drawPattern4, drawPattern5, drawPattern6};

/**************************************************************************
  Pintamos Patron
 **************************************************************************/
void restorePattern(int x, int y)
{
  int colBrick = x / _game.brick.width;
  int rowBrick = y / _game.brick.height;

  // Si hay ladrillo no pintamos
  // Estamos en la zona de ladrillos
  if(colBrick < BRICKS_COLS && rowBrick < BRICKS_ROWS)
  {
    int8_t brick = _game.bricks[colBrick][rowBrick];

    // Hay ladrillo, nos vamos
    if(brick > 0)
    {
      return;
    }
  }

  drawPattern(x, y);
}

/**************************************************************************
  Pintamos Patron
 **************************************************************************/
void drawPattern(int x, int y)
{
  x += _game.arenaPosition.x;
  y += _game.arenaPosition.y;

  // Pintamos el Patron que toque
  pDrawPatternFunction[_game.level % 6](x, y);
}

/**************************************************************************
  Pintamos Borde 1
 **************************************************************************/
void drawBorder1(int x, int y)
{
  drawLineV(x, y, 8, GRAY_DARK);
  drawLineV(x + 1, y, 8, GRAY);
  drawLineV(x + 2, y, 8, WHITE);
  drawLineV(x + 3, y, 8, GRAY);
  fillRect(x + 4, y, 3, 8, GRAY_DARK);
  drawLineV(x + 7, y, 8, BLACK);
}

/**************************************************************************
  Pintamos Borde 2
 **************************************************************************/
void drawBorder2(int x, int y)
{
  drawLineV(x, y, 8, GRAY);
  drawLineV(x + 1, y, 8, WHITE);
  fillRect(x + 2, y, 2, 8, GRAY);
  fillRect(x + 4, y, 3, 8, GRAY_DARK);
  drawLineV(x + 7, y, 8, GRAY);
}

/**************************************************************************
  Pintamos Borde 3
 **************************************************************************/
void drawBorder3(int x, int y)
{
  drawLineV(x, y, 8, GRAY);
  drawLineV(x + 1, y, 8, WHITE);
  fillRect(x + 2, y, 2, 8, GRAY);
  fillRect(x + 4, y, 3, 8, GRAY_DARK);
  drawLineV(x + 7, y, 8, GRAY);
  drawLineH(x, y + 1, 6, BLACK);
  drawLineH(x, y + 3, 6, BLACK);
  drawLineH(x, y + 5, 6, BLACK);
}

/**************************************************************************
  Pintamos Borde 4
 **************************************************************************/
void drawBorder4(int x, int y)
{
  drawLineH(x, y, 8, GRAY_DARK);
  drawLineH(x, y + 1, 8, GRAY);
  drawLineH(x, y + 2, 8, WHITE);
  drawLineH(x, y + 3, 8, GRAY);
  fillRect(x, y + 4, 8, 3, GRAY_DARK);
  drawLineH(x, y + 7, 8, BLACK);
}

/**************************************************************************
  Pintamos Borde 5
 **************************************************************************/
void drawBorder5(int x, int y)
{
  drawLineH(x, y, 8, GRAY);
  drawLineH(x, y + 1, 8, WHITE);
  fillRect(x, y + 2, 8, 2, GRAY);
  fillRect(x, y + 4, 8, 3, GRAY_DARK);
  drawLineH(x, y + 7, 8, GRAY);
}

/**************************************************************************
  Pintamos Borde 6
 **************************************************************************/
void drawBorder6(int x, int y)
{
  drawLineH(x, y, 8, GRAY);
  drawLineH(x, y + 1, 8, WHITE);
  fillRect(x, y + 2, 8, 2, GRAY);
  fillRect(x, y + 4, 8, 3, GRAY_DARK);
  drawLineH(x, y + 7, 8, GRAY);
  drawLineV(x + 1, y, 6, BLACK);
  drawLineV(x + 3, y, 6, BLACK);
  drawLineV(x + 5, y, 6, BLACK);
}

/**************************************************************************
  Pintamos Ladrillo
 **************************************************************************/
void drawBrick(int x, int y, uint8_t palette)
{
  x += _game.arenaPosition.x;
  y += _game.arenaPosition.y;

  fillRect(x + 1, y + 1, 14, 6, _palette[palette][0]);
  
  drawLineH(x, y, 16, WHITE);
  drawLineV(x, y, 6, WHITE);

  drawLineH(x, y + 7, 16, _palette[palette][1]);
  drawLineV(x + 15, y + 1, 6,  _palette[palette][1]);
}

/**************************************************************************
  Pintamos Ladrillo Duro
 **************************************************************************/
void drawBrickStrong(int x, int y, uint8_t palette)
{
  x += _game.arenaPosition.x;
  y += _game.arenaPosition.y;

  fillRect(x + 2, y + 2, 12, 6, _palette[palette][0]);
  
  drawLineH(x, y, 16, WHITE);
  drawLineV(x, y, 6, WHITE);
  drawLineH(x + 1, y + 1, 14, WHITE);
  drawLineV(x + 1, y + 1, 4, WHITE);

  drawLineH(x, y + 7, 16, _palette[palette][1]);
  drawLineV(x + 15, y + 1, 6,  _palette[palette][1]);
  drawLineH(x + 1, y + 6, 14, _palette[palette][1]);
  drawLineV(x + 14, y + 2, 4,  _palette[palette][1]);
}

/**************************************************************************
  Pintamos la Nave con borrado anterior
 **************************************************************************/
void drawShip(Point old, Point pos)
{
  int ox = old.x;
  int oy = old.y;
  int x = pos.x; 
  int y = pos.y;

  // Borramos con clipping el residuo anterior rellenando con Patron
  // Si vamos a la derecha
  if(ox < x)
  {
    // Activamos el Clipping, num negativos establece el clipping a la derecha
    _game.clipping.x = -(x + _game.arenaPosition.x);
    
    int cox = ox / _game.patternSize * _game.patternSize;
    int cx = x / _game.patternSize * _game.patternSize;

    for(int i = cox; i < cx + _game.patternSize; i += _game.patternSize)
    {
      drawPattern(i, y);
    }
  }
  // Si vamos a la izquierda
  if(ox > x)
  {
    // Activamos el Clipping, num positivos establece el clipping a la izquierda
    _game.clipping.x = x + _game.paddle.width + _game.arenaPosition.x - 2;

    int cox = (ox + _game.paddle.width)/ _game.patternSize * _game.patternSize;;
    int cx = (x + _game.paddle.width) / _game.patternSize * _game.patternSize;;

    for(int i = cx; i < cox + _game.patternSize; i += _game.patternSize)
    {
      drawPattern(i, y);
    }
  }

  // Pintamos la nave
  drawShip(x, y);
 
  // Desactivamos el clipping
  _game.clipping.x = 0;
}

/**************************************************************************
  Pintamos Nave
 **************************************************************************/
void drawShip(int x, int y)
{
  x += _game.arenaPosition.x; 
  y += _game.arenaPosition.y;

   // Cuerpo
  drawLineH(x + 3, y, 10, GRAY_DARK);
  drawLineH(x + 3, y + 1, 10, GRAY);
  drawLineH(x, y + 2, 16, WHITE);
  fillRect(x + 3, y + 3, 10, 2, GRAY);
  fillRect(x + 3, y + 5, 10, 2, GRAY_DARK);
  drawLineH(x, y + 7, 16, BLACK);

  // Terminador Izquierdo
  drawLineH(x, y, 3, RED_DARK);
  drawLineH(x, y + 1, 3, RED);
  fillRect(x, y + 3, 3, 2, RED);
  fillRect(x, y + 5, 3, 2, RED_DARK);  

  // Terminador Derecho
  drawLineH(x + 12, y, 3, RED_DARK);
  drawLineH(x + 12, y + 1, 3, RED);
  fillRect(x + 12, y + 3, 3, 2, RED);
  fillRect(x + 12, y + 5, 3, 2, RED_DARK);
}

/**************************************************************************
  Pintamos Bola
 **************************************************************************/
void drawBall(Point old, Point pos)
{
  int ox = old.x;
  int oy = old.y;
  int x = pos.x;
  int y = pos.y;

  removeBall(ox, oy);

  drawBall(x, y);
}

/**************************************************************************
  Borramos Bola
 **************************************************************************/
void removeBall(int ox, int oy)
{
  if(_game.counter == 0)
  {
    int remainder = _game.patternSize - _game.ballSize;

    int cox = (ox / _game.patternSize) * _game.patternSize;
    int coy = (oy / _game.patternSize) * _game.patternSize;

    restorePattern(cox, coy);
    if(ox % _game.patternSize > remainder)
    {
      restorePattern(cox + _game.patternSize, coy);

      if(oy % _game.patternSize > remainder)
      {
        restorePattern(cox + _game.patternSize, coy + _game.patternSize);
      }
    }
    if(oy % _game.patternSize > remainder)
    {
        restorePattern(cox, coy + _game.patternSize);
    }
  }
}

/**************************************************************************
  Pintamos Bola
 **************************************************************************/
void drawBall(int x, int y)
{
  x += _game.arenaPosition.x;
  y += _game.arenaPosition.y;

  drawLineH(x + 1, y, 2, GRAY);
  drawLineH(x + 1, y + 3, 2, GRAY_DARK);
  drawLineV(x, y + 1, 2, GRAY);
  drawLineV(x + 3, y + 1, 2, GRAY_DARK);
  fillRect(x + 1, y + 1, 2, 2, WHITE);
  drawLineH(x + 2, y + 2, 1, GRAY);
}

/**************************************************************************
  Pintamos Borde 
 **************************************************************************/
void drawBorder()
{
  int x, y, limi;

  // Calculo del limite de i en vertical
  limi = _game.screen.height / 8;

  // Pintamos los laterales
  for(int i = 1; i < limi; i ++)
  {
    y = i * 8;
    
    if(i % 5 == 1 || i % 5 == 3)
    {
      drawBorder2(0, y);
      drawBorder2(120, y);
    }
    else if(i % 5 == 2)
    {
      drawBorder3(0, y);
      drawBorder3(120, y);
    }
    else if(i % 5 == 0 || i % 5 == 4)
    {
      drawBorder1(0, y);
      drawBorder1(120, y);
    }
  }

  // Calculo del limite de i en horizontal
  limi = _game.screenHalf.width / 8;

  // Pintamos el superior
  for(int i = 1; i < limi; i ++)
  {
    x = i * 8;
    
    if(i % 4 == 2 || i % 4 == 0)
    {
      drawBorder5(x, 8);
      drawBorder5(120 - x, 8);
    }
    else if(i % 4 == 3)
    {
      drawBorder6(x, 8);
      drawBorder6(120 - x, 8);
    }
    else if(i % 4 == 1)
    {
      drawBorder4(x, 8);
      drawBorder4(120 - x, 8);
    }
  }
}

/**************************************************************************
  Pintamos BackGround 
 **************************************************************************/
void drawBackGround()
{
  for(int x = 0; x < _game.arenaSize.width; x += 8)
  {
    for(int y = 0; y < _game.arenaSize.height; y += 8)
    {
      drawPattern(x, y);
    }
  }
}

/**************************************************************************
  Pintamos Los ladrillos del nivel 
 **************************************************************************/
void drawBricksLevel()
{
  for(int col = 0; col < BRICKS_COLS; col++)
  {
    for(int row = 0; row < BRICKS_ROWS; row++)
    {
      uint8_t palette = _game.bricks[col][row];

      if(palette > 0)
      {
        if(palette < 7)
        {
          drawBrick(col * _game.brick.width, row * _game.brick.height,  palette);
        }
        else
        {
          drawBrickStrong(col * _game.brick.width, row * _game.brick.height,  palette);
        }
      }
    }
  }
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   D I B U J O   M A R C A D O R E S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos Etiqueta Score
 **************************************************************************/
void drawScoreLabel()
{
  drawText("SC", _game.lblPlayerScore.x, _game.lblPlayerScore.y, 1, RED);
}

/**************************************************************************
  Pintamos Score
 **************************************************************************/
void drawScore(uint16_t color)
{
  char buf[6];

  sprintf(buf, "%05d", _game.score);

  drawText(buf, _game.playerScore.x, _game.playerScore.y, 1, color);
}

/**************************************************************************
  Pintamos Etiqueta Level
 **************************************************************************/
void drawLevelLabel()
{
  drawText("L", _game.lblPlayerLevel.x, _game.lblPlayerLevel.y, 1, RED);
}

/**************************************************************************
  Pintamos Level
 **************************************************************************/
void drawLevel(uint16_t color)
{
  char buf[3];

  sprintf(buf, "%02d", _game.level + 1);

  drawText(buf, _game.playerLevel.x, _game.playerLevel.y, 1, color);
}

/**************************************************************************
  Pintamos Etiqueta Lifes
 **************************************************************************/
void drawLifesLabel()
{
  drawText("B", _game.lblPlayerLifes.x, _game.lblPlayerLifes.y, 1, RED);
}

/**************************************************************************
  Pintamos Lifes
 **************************************************************************/
void drawLifes(uint16_t color)
{
  char buf[2];

  sprintf(buf, "%01d", _game.lifes);

  drawText(buf, _game.playerLifes.x, _game.playerLifes.y, 1, color);
}

/**************************************************************************
  Establecemos Score
 **************************************************************************/
void setScore(uint16_t scr)
{
  drawScore(BLACK);

  // Vida Extra si tenemos menos de 9
  if(_game.score / EXTRA_BALL < scr / EXTRA_BALL && _game.lifes < 9)
  {
     setLifes(_game.lifes + 1);

     soundExtraLife();
  }

  _game.score = scr;

  drawScore(WHITE);
}

/**************************************************************************
  Establecemos Nivel
 **************************************************************************/
void setLevel(uint16_t lvl)
{
  drawLevel(BLACK);

  // Maximo 15 niveles, damos la vuelta
  _game.level = lvl % 15;

  drawLevel(WHITE);
}

/**************************************************************************
  Establecemos Vidas
 **************************************************************************/
void setLifes(uint16_t lif)
{
  drawLifes(BLACK);

  _game.lifes = lif;

  drawLifes(WHITE);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E  A B S T R A C C I O N   G R A F I C A
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos Rectangulo relleno
 **************************************************************************/
void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
  tft.fillRect(x, y, width, height, color);
}

/**************************************************************************
  Pintamos linea Horizontal
 **************************************************************************/
void drawLineH(int16_t x, int16_t y, int16_t width, uint16_t color)
{
  tft.drawFastHLine(x, y, width, color);
}

/**************************************************************************
  Pintamos linea Vertical
 **************************************************************************/
void drawLineV(int16_t x, int16_t y, int16_t height, uint16_t color)
{
  tft.drawFastVLine(x, y, height, color);
}

/**************************************************************************
  indicamos si clipping total en esa coordenada
 **************************************************************************/
bool clippingX(int16_t x)
{
  // Clipping 0 significa clipping desactivado
  if(_game.clipping.x == 0)
  {
    return false;
  }

  // Clipping negativo indica clipping a la derecha
  // Clipping total, lo indicamos para que no pinte
  if(_game.clipping.x < 0 && x >= abs(_game.clipping.x))
  {
    return true;
  }

  // Clipping positivo indica clipping a la izquierda
  // Clipping total, lo indicamos para que no pinte
  if(_game.clipping.x > 0 && x <= _game.clipping.x)
  {
    return true;
  }

  // No hay clipping
  return false;
}

/**************************************************************************
  indicamos si clipping total en esa coordenada, tamaño, sino 
  devuelve los datos ajustados 
 **************************************************************************/
bool clippingX(int16_t *x, int16_t *width)
{
  // Clipping 0 significa clipping desactivado
  if(_game.clipping.x == 0)
  {
    return false;
  }

  // Clipping negativo indica clipping a la derecha
  if(_game.clipping.x < 0)
  {
    // Clipping total, lo indicamos para que no pinte
    if(*x >= abs(_game.clipping.x))
    {
      return true;
    }
    
    // Clipping parcial cambiamos valores para adecuarlos al clipping
    if(*x + *width >= abs(_game.clipping.x))
    {
      *width = abs(_game.clipping.x) - *x;

      return false; 
    }

    // No hay clipping
    return false;
  }

  // Clipping positivo indica clipping a la izquierda
  if(_game.clipping.x > 0)
  {
    // Clipping total, lo indicamos para que no pinte
    if(*x + *width <= _game.clipping.x)
    {
      return true;
    }
    
    // Clipping parcial cambiamos valores para adecuarlos al clipping
    if(*x <= _game.clipping.x)
    {
      *width = *x + *width - _game.clipping.x - 1;
      *x = _game.clipping.x + 1;
      
      return false; 
    }

    // No hay clipping
    return false;
  }

  // No hay clipping
  return false;
}

/**************************************************************************
  indicamos si clipping total en esa coordenada
 **************************************************************************/
bool clippingY(int16_t y)
{
  // Clipping 0 significa clipping desactivado
  if(_game.clipping.y == 0)
  {
    return false;
  }

  // Clipping negativo indica clipping a la derecha
  // Clipping total, lo indicamos para que no pinte
  if(_game.clipping.y < 0 && y >= abs(_game.clipping.y))
  {
    return true;
  }

  // Clipping positivo indica clipping a la izquierda
  // Clipping total, lo indicamos para que no pinte
  if(_game.clipping.y > 0 && y <= _game.clipping.y)
  {
    return true;
  }

  // No hay clipping
  return false;
}

/**************************************************************************
  indicamos si clipping total en esa coordenada, tamaño, sino 
  devuelve los datos ajustados 
 **************************************************************************/
bool clippingY(int16_t *y, int16_t *height)
{
  // Clipping 0 significa clipping desactivado
  if(_game.clipping.y == 0)
  {
    return false;
  }

  // Clipping negativo indica clipping abajo
  if(_game.clipping.y < 0)
  {
    // Clipping total, lo indicamos para que no pinte
    if(*y >= abs(_game.clipping.y))
    {
      return true;
    }
    
    // Clipping parcial cambiamos valores para adecuarlos al clipping
    if(*y + *height >= abs(_game.clipping.y))
    {
      *height = abs(_game.clipping.y) - *y;

      return false; 
    }

    // No hay clipping
    return false;
  }

  // Clipping positivo indica clipping arriba
  if(_game.clipping.y > 0)
  {
    // Clipping total, lo indicamos para que no pinte
    if(*y + *height <= _game.clipping.y)
    {
      return true;
    }
    
    // Clipping parcial cambiamos valores para adecuarlos al clipping
    if(*y <= _game.clipping.y)
    {
      *height = *y + *height - _game.clipping.y - 1;
      *y = _game.clipping.y + 1;
      
      return false; 
    }

    // No hay clipping
    return false;
  }

  // No hay clipping
  return false;
}

/**************************************************************************
  Pintamos Rectangulo relleno con clipping
 **************************************************************************/
void fillRectWithClipping(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
  if(clippingX(&x, &width) == true)
  {
    return;
  }

  tft.fillRect(x, y, width, height, color);
}

/**************************************************************************
  Pintamos linea Horizontal con clipping
 **************************************************************************/
void drawLineHWithClipping(int16_t x, int16_t y, int16_t width, uint16_t color)
{
  if(clippingX(&x, &width))
  {
    return;
  }

  tft.drawFastHLine(x, y, width, color);
}

/**************************************************************************
  Pintamos linea Vertical con clipping
 **************************************************************************/
void drawLineVWithClipping(int16_t x, int16_t y, int16_t height, uint16_t color)
{
  if(clippingX(x))
  {
    return;
  }

  tft.drawFastVLine(x, y, height, color);
}

/**************************************************************************
  Pintamos Texto
 **************************************************************************/
void drawText(const char *text, int16_t x, int16_t y, uint8_t size, uint16_t color) 
{
  tft.setCursor(x, y);
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.print(text);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   A U D I O
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Sonido de pelota
 **************************************************************************/
void soundBall(unsigned int tono)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  tone(BUZZER, tono, 15);
}

/**************************************************************************
  Sonido de Empezar Partida
 **************************************************************************/
void soundPlay()
{
  for(int i = 0; i < 50; i++)
  {
    tone(BUZZER, 2000 + i * 10 , 30);
  }
}

/**************************************************************************
  Sonido de Vida Extra
 **************************************************************************/
void soundExtraLife()
{
  for(int i = 0; i < 200; i++)
  {
    tone(BUZZER, 1000 + i * 5 , 30);
  }
}

