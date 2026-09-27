#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream> 
#include <chrono>
#include <string>
#include <SDL_Image.h>
#include <SDL_mixer.h>
#include <time.h> 

using namespace std;

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const int BALL_WIDTH = 40;
const int BALL_HEIGHT = 40;
const int PADDLE_WIDTH = 40;
const int PADDLE_HEIGHT = 100;
const float OG_BALLSPEED = 1.0f;
float BALL_SPEED = 1.0f;
float PADDLE_SPEED = 1.2f;
const float CPU_PADDLE_SPEED = 0.9f; // Movimiento de IA. Mayor valor = Paleta más rápida.
const float CPU_REACTION = 20.0f; // Para que la paleta no se mueva tanto cuando tenga la pelotita al alcance. Mayor valor = Más pelotas pierde.
const float CPU_SMOOTH = 0.10f; // Suavizado de movimiento. Mayor valor = Mejor reacción de la IA.

enum Buttons // Inputs de botones
{
	PaddleOneUp = 0,
	PaddleOneDown
};

enum class MenuOption { Play, Exit }; // Opciones del menú

// Muestro intro de "estudio de desarrollo"
void showStudioScreen(SDL_Renderer* renderer, TTF_Font* fuente, SDL_Texture* logoTex) {
	const int SPACING = 10;  // separación entre logo y texto

	// Pongo y mido texto
	const char* mensaje = "STUDIOS";
	int textW, textH;
	TTF_SizeText(fuente, mensaje, &textW, &textH);

	// Crear la textura de texto
	SDL_Color blackFont = { 0,0,0,255 };
	SDL_Surface* surf = TTF_RenderText_Blended(fuente, mensaje, blackFont);
	SDL_Texture* textTex = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);
	SDL_SetTextureBlendMode(textTex, SDL_BLENDMODE_BLEND); // Blendmode es para efecto fade del logo
	SDL_SetTextureBlendMode(logoTex, SDL_BLENDMODE_BLEND);

	// Calcular tamaño de logo de modo que logo + texto + SPACING < WINDOW_HEIGHT
	int maxLogoH = WINDOW_HEIGHT - SPACING - textH;

	int logoW = WINDOW_WIDTH / 4; // Ancho de logo
	int logoH = 200;			  // Alto de logo

	// Calcular tamaño de bloque de logo y texto + Y para centrar
	int blockH = logoH + SPACING + textH;
	int blockY = (WINDOW_HEIGHT - blockH) / 2;

	// Posición de logo y texto
	SDL_Rect logoDest = {
		(WINDOW_WIDTH - logoW) / 2 - 15,
		blockY,
		logoW,
		logoH
	};
	SDL_Rect textDest = {
		(WINDOW_WIDTH - textW) / 2 - 20, // Ajusto la posición del texto un poquito con -10
		blockY + logoH,
		textW,
		textH
	};

	// Junto las funciones de render para no copiar y pegar tanto texto
	auto renderIntro = [&](int alpha) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);
		SDL_SetTextureAlphaMod(logoTex, alpha);
		SDL_SetTextureAlphaMod(textTex, alpha);
		SDL_RenderCopy(renderer, logoTex, nullptr, &logoDest);
		SDL_RenderCopy(renderer, textTex, nullptr, &textDest);
		SDL_RenderPresent(renderer);
		};

	// Fade-in
	for (int a = 0; a <= 255; a += 5) {
		renderIntro(a);
		SDL_Delay(10);
	}

	SDL_Delay(1350); // Tiempo del logo en pantalla

	// Fade-out
	for (int a = 255; a >= 0; a -= 5) {
		renderIntro(a);
		SDL_Delay(10);
	}

	SDL_DestroyTexture(textTex);
}

// Menú Principal
bool ShowMenu(SDL_Renderer* renderer, TTF_Font* titleFont, TTF_Font* menuFont, SDL_Texture* bgTex, const int winW, const int winH) { // Menú Principal
	SDL_Color titleColor = { 255, 255, 0, 255 };     // Color para el título
	SDL_Color on = { 255, 140, 0, 255 };     // Color cuando está seleccionado
	SDL_Color off = { 255, 255, 255, 255 };   // Color normal
	SDL_Color keys = { 0, 0, 0, 255 }; // Color de leyendas
	int selection = 0;                     // 0 = Jugar, 1 = Salir
	SDL_Event e;

	// Obtengo dimensiones de la ventana para centrar
	int w, h;
	SDL_GetRendererOutputSize(renderer, &w, &h);

	while (true) {
		// ─── Eventos ─────────────────────────────
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) return false;
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_s) {
					selection = 1 - selection;
				}
				else if (e.key.keysym.sym == SDLK_RETURN) {
					return (selection == 0);
				}
			}
		}

		// Dibujo fondo (imagen)
		int bgW, bgH;
		SDL_QueryTexture(bgTex, nullptr, nullptr, &bgW, &bgH);
		SDL_Rect bgDst;
		bgDst.w = bgW;
		bgDst.h = bgH;
		bgDst.x = (winW - bgW) / 2;
		bgDst.y = (winH - bgH) / 2;

		SDL_RenderCopy(renderer, bgTex, nullptr, &bgDst);

		// Título
		SDL_Surface* surfTitle = TTF_RenderText_Solid(titleFont, "PONG PLAYA", titleColor);
		SDL_Texture* texTitle = SDL_CreateTextureFromSurface(renderer, surfTitle);
		SDL_Rect     rTitle = {
			(w - surfTitle->w) / 2,   // X centrado
			h / 8,                    // Y a 1/8 de la pantalla
			surfTitle->w,
			surfTitle->h
		};
		SDL_RenderCopy(renderer, texTitle, nullptr, &rTitle);
		SDL_DestroyTexture(texTitle);
		SDL_FreeSurface(surfTitle);

		// Opción "JUGAR"
		SDL_Surface* surfPlay = TTF_RenderText_Solid(menuFont, "JUGAR", (selection == 0) ? on : off);
		SDL_Texture* texPlay = SDL_CreateTextureFromSurface(renderer, surfPlay);
		SDL_Rect     rPlay = {
			(w - surfPlay->w) / 2,  // X centrado
			h / 2 - surfPlay->h,    // Y justo arriba del medio
			surfPlay->w,
			surfPlay->h
		};
		SDL_RenderCopy(renderer, texPlay, nullptr, &rPlay);
		SDL_DestroyTexture(texPlay);
		SDL_FreeSurface(surfPlay);

		// Opción "SALIR"
		SDL_Surface* surfExit = TTF_RenderText_Solid(menuFont, "SALIR", (selection == 1) ? on : off);
		SDL_Texture* texExit = SDL_CreateTextureFromSurface(renderer, surfExit);
		SDL_Rect     rExit = {
			(w - surfExit->w) / 2,
			h / 2 + surfExit->h,    // Y justo abajo del medio
			surfExit->w,
			surfExit->h
		};
		SDL_RenderCopy(renderer, texExit, nullptr, &rExit);
		SDL_DestroyTexture(texExit);
		SDL_FreeSurface(surfExit);

		// ─── Dibujo leyenda de controles (parte inferior izquierda) ────────────────

		// Defino el color para las instrucciones y márgenes
		SDL_Color hintColor = { 0, 0, 0, 255 };  // Leyenda en color negro
		int marginLeft = 20;              // Margen desde la izquierda
		int marginBottom = 20;              // Margen desde el fondo
		int spacing = 5;               // Espacio entre líneas

		// Leyenda "W: Arriba"
		SDL_Surface* surfW = TTF_RenderText_Solid(menuFont, "W: Arriba", hintColor);
		SDL_Texture* texW = SDL_CreateTextureFromSurface(renderer, surfW);
		// Se ubica la instrucción en la esquina inferior izquierda:
		// El bloque de instrucciones ocupará tres líneas, así que se posiciona la primera
		SDL_Rect rW = {
			marginLeft,                           // x: a 20 px del borde izquierdo
			h - marginBottom - (surfW->h * 3) - spacing * 2, // y: calculado desde abajo. Multiplico por 3 para que haya espacio para las 3 leyendas
			surfW->w,
			surfW->h
		};
		SDL_RenderCopy(renderer, texW, nullptr, &rW);
		SDL_DestroyTexture(texW);
		SDL_FreeSurface(surfW);

		// Leyenda "S: Abajo"
		SDL_Surface* surfS = TTF_RenderText_Solid(menuFont, "S: Abajo", hintColor);
		SDL_Texture* texS = SDL_CreateTextureFromSurface(renderer, surfS);
		SDL_Rect rS = {
			marginLeft,                       // misma x que la anterior
			rW.y + surfS->h + spacing,        // se coloca justo debajo de "W: Arriba"
			surfS->w,
			surfS->h
		};
		SDL_RenderCopy(renderer, texS, nullptr, &rS);
		SDL_DestroyTexture(texS);
		SDL_FreeSurface(surfS);

		// Leyenda "ENTER: confirmar"
		SDL_Surface* surfEnter = TTF_RenderText_Solid(menuFont, "ENTER: confirmar", hintColor);
		SDL_Texture* texEnter = SDL_CreateTextureFromSurface(renderer, surfEnter);
		SDL_Rect rEnter = {
			marginLeft,                       // misma x
			rS.y + surfEnter->h + spacing,    // debajo de la instrucción anterior
			surfEnter->w,
			surfEnter->h
		};
		SDL_RenderCopy(renderer, texEnter, nullptr, &rEnter);
		SDL_DestroyTexture(texEnter);
		SDL_FreeSurface(surfEnter);

		SDL_RenderPresent(renderer);
	}
}

// Pantalla de fin de partida
void showEndMatch(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* font2, const string& mensaje) // Función que muestra el mensaje de fin de partida y espera ENTER
{
	// Render del texto
	SDL_Color Blue = { 0,100,255,255 };
	SDL_Surface* surfResult = TTF_RenderText_Blended(font, mensaje.c_str(), Blue);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surfResult);
	SDL_FreeSurface(surfResult);

	int w, h;
	TTF_SizeText(font, mensaje.c_str(), &w, &h);
	SDL_Rect dst = { (WINDOW_WIDTH - w) / 2, (WINDOW_HEIGHT - h) / 2 - 50, w, h };
	SDL_RenderCopy(renderer, tex, nullptr, &dst);
	SDL_DestroyTexture(tex);

	// Instrucción “ENTER para volver”
	const char* instr = "ENTER: Volver al menu";
	SDL_Color black = { 0,0,0,255 };
	SDL_Surface* surfInst = TTF_RenderText_Blended(font2, instr, black);
	tex = SDL_CreateTextureFromSurface(renderer, surfInst);
	SDL_FreeSurface(surfInst);

	TTF_SizeText(font2, instr, &w, &h);
	SDL_Rect dst2 = { (WINDOW_WIDTH - w) / 2, (WINDOW_HEIGHT - h) / 2 + 20, w, h };
	SDL_RenderCopy(renderer, tex, nullptr, &dst2);
	SDL_DestroyTexture(tex);

	SDL_RenderPresent(renderer);

	// Espera ENTER
	SDL_Event e;
	bool wait = true;
	while (wait) {
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_KEYDOWN &&
				(e.key.keysym.sym == SDLK_RETURN ||
					e.key.keysym.sym == SDLK_KP_ENTER))
			{
				wait = false;
			}
		}
	}
}

// Guarda: Resultado;Puntos Jugador;Puntos CPU
void saveRecord(string filename, int scorePJ, int scoreCPU)
{
	ofstream archive(filename + ".csv", ios::app);

	// Determinar resultado
	string result;
	if (scorePJ > scoreCPU) result = "Victoria";
	else if (scorePJ < scoreCPU) result = "Derrota";
	else                          result = "Empate";

	// Volcar datos (sin encabezado nunca)
	archive << result << ';'
		<< scorePJ << ';'
		<< scoreCPU << '\n';
}

enum class CollisionType
{
	None,
	Top,
	Middle,
	Bottom,
	Left,
	Right
};

struct Contact // Agrupo datos bajo un mismo nombre
{
	CollisionType type;
	float penetration;
};

struct ScoreEntry {
	int   playerScore;
	int   cpuScore;
	float timeSeconds;
	bool  playerWon;
};

class Vec2 // Defino un vector bidimensional (matriz) donde operará el juego
{
public: // Lo hago accesible para todo el código
	Vec2()
		: x(0.0f), y(0.0f)
	{
	}

	Vec2(float x, float y)
		: x(x), y(y)
	{
	}

	Vec2 operator+(Vec2 const& rhs)
	{
		return Vec2(x + rhs.x, y + rhs.y);
	}

	Vec2& operator+=(Vec2 const& rhs)
	{
		x += rhs.x;
		y += rhs.y;

		return *this;
	}

	Vec2 operator*(float rhs)
	{
		return Vec2(x * rhs, y * rhs);
	}

	float x, y;
};

class Ball // Pelotita
{
public:
	Ball(Vec2 position, Vec2 velocity, SDL_Texture* ballspr) // SDL Texture seria el sprite del objeto, en este caso la pelotita
		: position(position), velocity(velocity), ballspr(ballspr)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = BALL_WIDTH;
		rect.h = BALL_HEIGHT;
	}

	void Update(float dt) // Actualizo posición de la pelotita
	{
		position += velocity * dt;
	}

	void Draw(SDL_Renderer* renderer) // Dibujo pelotita en posición nueva
	{
		rect.x = static_cast<int>(position.x); // Transformo numero a 'int' para evitar posibles errores
		rect.y = static_cast<int>(position.y);

		SDL_RenderCopy(renderer, ballspr, nullptr, &rect);
	}

	void CollideWithPaddle(Contact const& contact)
	{
		position.x += contact.penetration;
		velocity.x = -velocity.x;

		if (contact.type == CollisionType::Top)
		{
			velocity.y = -.60f * BALL_SPEED;
			velocity.x = (velocity.x > 0) ? BALL_SPEED : -BALL_SPEED;
		}
		else if (contact.type == CollisionType::Bottom)
		{
			velocity.y = 0.60f * BALL_SPEED;
			velocity.x = (velocity.x > 0) ? BALL_SPEED : -BALL_SPEED;
		}
		else



		{
			float deviation = (rand() % 2 == 0) ? 0.05f : -0.05f;
			velocity.y = deviation * BALL_SPEED;
			velocity.x = (velocity.x > 0) ? BALL_SPEED : -BALL_SPEED;
		}
	}

	void CollideWithWall(Contact const& contact)
	{
		if ((contact.type == CollisionType::Top)
			|| (contact.type == CollisionType::Bottom))
		{
			position.y += contact.penetration;
			velocity.y = -velocity.y;
		}
		else if (contact.type == CollisionType::Left)
		{
			position.x = WINDOW_WIDTH / 2.0f;
			position.y = WINDOW_HEIGHT / 2.0f;
			velocity.x = OG_BALLSPEED;
			velocity.y = 0;
		}
		else if (contact.type == CollisionType::Right)
		{
			position.x = WINDOW_WIDTH / 2.0f;
			position.y = WINDOW_HEIGHT / 2.0f;
			velocity.x = -OG_BALLSPEED;
			velocity.y = 0;
		}
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect{};
	SDL_Texture* ballspr;
};

class Paddle // Paletas
{
public:
	Paddle(Vec2 position, Vec2 velocity, SDL_Texture* sprite)
		: position(position), velocity(velocity), texture(sprite)

	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = PADDLE_WIDTH;
		rect.h = PADDLE_HEIGHT;
	}

	void UpdateAI(const Ball& ball, float dt_ms)
	{
		float paddleCenter = position.y + PADDLE_HEIGHT * 0.5f;
		float ballCenter = ball.position.y + BALL_HEIGHT * 0.5f;
		float diff = ballCenter - paddleCenter;
		float absDiff = (diff < 0) ? -diff : diff;

		if (absDiff < CPU_REACTION)
			velocity.y = 0.0f;
		else {
			float desiredVel = diff * CPU_SMOOTH * CPU_PADDLE_SPEED;
			if (desiredVel > CPU_PADDLE_SPEED)      desiredVel = CPU_PADDLE_SPEED;
			else if (desiredVel < -CPU_PADDLE_SPEED) desiredVel = -CPU_PADDLE_SPEED;
			velocity.y = desiredVel;
		}

		position.y += velocity.y * dt_ms;

		if (position.y < 0.0f)
			position.y = 0.0f;
		else if (position.y > WINDOW_HEIGHT - PADDLE_HEIGHT)
			position.y = WINDOW_HEIGHT - PADDLE_HEIGHT;

		rect.y = static_cast<int>(position.y);
	}

	void Update(float dt)
	{
		position += velocity * dt;

		if (position.y < 0)
		{
			// Frena en el borde superior
			position.y = 0;
		}
		else if (position.y > (WINDOW_HEIGHT - PADDLE_HEIGHT))
		{
			// Frena en el borde inferior
			position.y = WINDOW_HEIGHT - PADDLE_HEIGHT;
		}
	}

	void Draw(SDL_Renderer* renderer)
	{
		rect.y = static_cast<int>(position.y);

		SDL_RenderCopy(renderer, texture, nullptr, &rect); // Renderizo con el sprite cargado
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect{};
	SDL_Texture* texture; // Sprite de la paleta
};

class PlayerScore // Puntaje
{
public:
	PlayerScore(Vec2 position, SDL_Renderer* renderer, TTF_Font* font)
		: renderer(renderer), font(font)
	{
		surface = TTF_RenderText_Solid(font, "0", { 0, 0, 0, 255 });
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;
	}

	~PlayerScore()
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
	}

	void SetScore(int score) // Voy cambiando los puntos y los muestro a medida que los jugadores anotan
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);

		surface = TTF_RenderText_Solid(font, std::to_string(score).c_str(), { 0, 0, 0, 255 });
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		rect.w = width;
		rect.h = height;
	}

	void Draw()
	{
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
	}

	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Surface* surface{};
	SDL_Texture* texture{};
	SDL_Rect rect{};
};

Contact CheckPaddleCollision(Ball const& ball, Paddle const& paddle) // Colisiones de paletas y pelotita
{
	float ballLeft = ball.position.x;
	float ballRight = ball.position.x + BALL_WIDTH;
	float ballTop = ball.position.y;
	float ballBottom = ball.position.y + BALL_HEIGHT;

	float paddleLeft = paddle.position.x;
	float paddleRight = paddle.position.x + PADDLE_WIDTH;
	float paddleTop = paddle.position.y;
	float paddleBottom = paddle.position.y + PADDLE_HEIGHT;

	Contact contact{};

	// La pelotita sigue de largo si no colisiona con las paletas
	if (ballLeft >= paddleRight)
	{
		return contact;
	}

	if (ballRight <= paddleLeft)
	{
		return contact;
	}

	if (ballTop >= paddleBottom)
	{
		return contact;
	}

	if (ballBottom <= paddleTop)
	{
		return contact;
	}

	float paddleRangeUpper = paddleBottom - (2.0f * PADDLE_HEIGHT / 3.0f);
	float paddleRangeMiddle = paddleBottom - (PADDLE_HEIGHT / 3.0f);

	if (ball.velocity.x < 0) // La pelotita va para la izquierda
	{
		// Paleta izquierda
		contact.penetration = paddleRight - ballLeft;
	}
	else if (ball.velocity.x > 0) // La pelotita va para la derecha
	{
		// Paleta derecha
		contact.penetration = paddleLeft - ballRight;
	}

	// dirección del rebote
	if ((ballBottom > paddleTop)
		&& (ballBottom < paddleRangeUpper))
	{
		contact.type = CollisionType::Top; // Rebote para arriba
	}
	else if ((ballBottom > paddleRangeUpper)
		&& (ballBottom < paddleRangeMiddle))
	{
		contact.type = CollisionType::Middle; // Rebote recto
	}
	else
	{
		contact.type = CollisionType::Bottom; // Rebote para abajo
	}

	return contact;
}

Contact CheckWallCollision(Ball const& ball)
{
	float ballLeft = ball.position.x;
	float ballRight = ball.position.x + BALL_WIDTH;
	float ballTop = ball.position.y;
	float ballBottom = ball.position.y + BALL_HEIGHT;

	Contact contact{};

	if (ballLeft < 0.0f)
	{
		contact.type = CollisionType::Left;
	}
	else if (ballRight > WINDOW_WIDTH)
	{
		contact.type = CollisionType::Right;
	}
	else if (ballTop < 0.0f)
	{
		contact.type = CollisionType::Top;
		contact.penetration = -ballTop;
	}
	else if (ballBottom > WINDOW_HEIGHT)
	{
		contact.type = CollisionType::Bottom;
		contact.penetration = WINDOW_HEIGHT - ballBottom;
	}

	return contact;
}

int main(int argc, char* args[])
{
	srand(static_cast<unsigned int>(time(nullptr)));
	// Inicializo SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

	SDL_Window* window = SDL_CreateWindow("Pong Playa", 150, 100, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN); // Creo la ventana y le pongo el título acá
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); // Renderizo ventana
	TTF_Font* titleFont = TTF_OpenFont("../../Assets/Fonts/PlatinumSign.ttf", 50); // Elijo las fuentes-TTF
	TTF_Font* menuFont = TTF_OpenFont("../../Assets/Fonts/mono1001.ttf", 40);
	TTF_Font* resultFont = TTF_OpenFont("../../Assets/Fonts/bubblegum.ttf", 90);
	TTF_Font* introFont = TTF_OpenFont("../../Assets/Fonts/studios.ttf", 50);
	SDL_Texture* paddleSprite = IMG_LoadTexture(renderer, "../../Assets/Pictures/Lsaver1.png"); // Paleta = torpedo salvavidas
	SDL_Texture* ballSprite = IMG_LoadTexture(renderer, "../../Assets/Pictures/miniBall.png"); // Pelotita = pelota de playa
	SDL_Texture* studioLogo = IMG_LoadTexture(renderer, "../../Assets/Pictures/uadeLogo.png"); // Logo de UADE en la intro
	SDL_Texture* menuBgTex = IMG_LoadTexture(renderer, "../../Assets/Background/aguaBG.jpg"); // Fondo del menú: agua de playa
	SDL_Texture* gameBgTex = IMG_LoadTexture(renderer, "../../Assets/Background/arenaBG.jpeg"); // Fondo de la partida de Pong: arena de playa
	Mix_Chunk* wallHitSound = Mix_LoadWAV("../../Assets/Sounds/wallhit.wav"); 	// Cargo efectos de sonido antes del bucle de juego
	Mix_Chunk* paddleHitSound = Mix_LoadWAV("../../Assets/Sounds/clap-pp.aiff");
	Mix_Music* music = Mix_LoadMUS("../../Assets/Sounds/KD-ST.wav"); 	// Cargo la música
	if (!music) { // Chequeo que cargue la música
		SDL_Log("No se pudo cargar la música: %s", Mix_GetError());
	}
	// Chequeo que cargue los sonidos
	if (!wallHitSound || !paddleHitSound) {
		std::cerr << "Error al cargar sonidos: " << Mix_GetError() << std::endl;
		// Liberar fuentes o cualquier otro recurso cargado antes de salir
		return -1;
	}

	showStudioScreen(renderer, introFont, studioLogo); // Muestro intro
	SDL_DestroyTexture(studioLogo);
	Mix_PlayMusic(music, -1);  // -1 para que la música suene en loop desde el menú

	bool appRunning = true; // Mantengo andando la App
	while (appRunning)
	{
		bool volverAJugar = ShowMenu(renderer, titleFont, menuFont, menuBgTex, WINDOW_WIDTH, WINDOW_HEIGHT); // Muestro menú

		if (!volverAJugar)  // Si en el menú elige “Salir”
		{
			appRunning = false; // Salimos del juego
			return 0;
		}

		chrono::high_resolution_clock::time_point startTime; // Declaración fuera del loop
		MenuOption selected = MenuOption::Play;
		// Game loop
		{
			// Creo el lugar donde se mostrarán los puntajes
			PlayerScore playerOneScoreText(Vec2(WINDOW_WIDTH / 4, 20), renderer, menuFont);

			PlayerScore cpuScoreText(Vec2(3 * WINDOW_WIDTH / 4, 20), renderer, menuFont);

			// Creo la pelotita, con dimensiones, velocidad y sprite
			Ball ball(
				Vec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f),
				Vec2(BALL_SPEED, 0.0f),
				ballSprite);

			// Creo las paletas, con dimensiones, velocidad y sprite
			Paddle paddleOne(
				Vec2(10.0f, WINDOW_HEIGHT / 2.0f),
				Vec2(0.0f, 0.0f),
				paddleSprite);

			Paddle paddleCPU(
				Vec2(WINDOW_WIDTH - 50.0f, WINDOW_HEIGHT / 2.0f),
				Vec2(0.0f, 0.0f),
				paddleSprite);

			// Puntajes iniciales
			int playerOneScore = 0;
			int cpuScore = 0;

			bool running = true;
			bool buttons[2] = {};

			float dt = 0.0f;
			float  timeRemain = 120.0f;   // timer de la partida en segundos
			int    maxPoints = 7; // Condición de victoria

			while (running) // Corre del juego hasta que se cierra
			{
				auto startTime = chrono::high_resolution_clock::now();

				SDL_Event event;
				while (SDL_PollEvent(&event))
				{
					if (event.type == SDL_QUIT)
					{
						running = false;
					}
					else if (event.type == SDL_KEYDOWN)
					{
						if (event.key.keysym.sym == SDLK_ESCAPE)
						{
							running = false;
						}
						else if (event.key.keysym.sym == SDLK_w)
						{
							buttons[Buttons::PaddleOneUp] = true;
						}
						else if (event.key.keysym.sym == SDLK_s)
						{
							buttons[Buttons::PaddleOneDown] = true;
						}
					}
					else if (event.type == SDL_KEYUP)
					{
						if (event.key.keysym.sym == SDLK_w)
						{
							buttons[Buttons::PaddleOneUp] = false;
						}
						else if (event.key.keysym.sym == SDLK_s)
						{
							buttons[Buttons::PaddleOneDown] = false;
						}
					}
				}

				// Tiempo restante de la partida
				timeRemain -= dt / 1000.0f;
				if (timeRemain < 0.0f) timeRemain = 0.0f;

				// Movimiento de las paletas
				if (buttons[Buttons::PaddleOneUp])
				{
					paddleOne.velocity.y = -PADDLE_SPEED;
				}
				else if (buttons[Buttons::PaddleOneDown])
				{
					paddleOne.velocity.y = PADDLE_SPEED;
				}
				else
				{
					paddleOne.velocity.y = 0.0f;
				}

				// Actualizo la posición de las paletas
				paddleOne.Update(dt);
				paddleCPU.UpdateAI(ball, dt);

				// Actualizo la posición de la pelota
				ball.Update(dt);

				// Chequeo colisiones
				if (Contact contact = CheckPaddleCollision(ball, paddleOne);
					contact.type != CollisionType::None)
				{
					ball.CollideWithPaddle(contact);

					BALL_SPEED += 0.09f;

					Mix_PlayChannel(-1, paddleHitSound, 0); // Ejecuto el sonido
				}
				else if (contact = CheckPaddleCollision(ball, paddleCPU),
					contact.type != CollisionType::None)
				{
					ball.CollideWithPaddle(contact);

					BALL_SPEED += 0.09f;

					Mix_PlayChannel(-1, paddleHitSound, 0);
				}
				else if (contact = CheckWallCollision(ball),
					contact.type != CollisionType::None)
				{
					ball.CollideWithWall(contact);

					// Sumo puntos al jugador que corresponda
					if (contact.type == CollisionType::Left)
					{
						++cpuScore;
						BALL_SPEED = OG_BALLSPEED;
						cpuScoreText.SetScore(cpuScore);
					}
					else if (contact.type == CollisionType::Right)
					{
						++playerOneScore;
						BALL_SPEED = OG_BALLSPEED;
						playerOneScoreText.SetScore(playerOneScore);
					}
					else
					{
						Mix_PlayChannel(-1, wallHitSound, 0);
					}
				}

				// Condiciones de fin de partida
				if (playerOneScore >= maxPoints || cpuScore >= maxPoints) {
					running = false;
				}
				// Chequeo fin por tiempo
				else if (timeRemain <= 0.0f) {
					running = false;
				}

				SDL_RenderClear(renderer); // Limpio ventana antes de renderizar
				SDL_RenderCopy(renderer, gameBgTex, nullptr, nullptr);
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Pinto elementos de negro

				// Dibujo la red
				for (int y = 0; y < WINDOW_HEIGHT; ++y)
				{
					if (y % 5) // Salteo pixeles de 'y' para dar efecto de red
					{
						SDL_RenderDrawPoint(renderer, WINDOW_WIDTH / 2, y);
					}
				}

				ball.Draw(renderer); // Dibujo la pelotita

				// Dibujo las paletas
				paddleOne.Draw(renderer);
				paddleCPU.Draw(renderer);

				// Muestro puntajes
				playerOneScoreText.Draw();
				cpuScoreText.Draw();


				// Dibujo timer en pantalla (superior centro)
				{
					int secs = static_cast<int>(timeRemain);
					string tmp = "Tiempo: " + to_string(secs);
					SDL_Color col = { 0,0,0,255 };
					SDL_Surface* surf = TTF_RenderText_Blended(menuFont, tmp.c_str(), col);
					SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
					SDL_FreeSurface(surf);

					int tw, th;
					TTF_SizeText(menuFont, tmp.c_str(), &tw, &th);
					SDL_Rect dst = { (WINDOW_WIDTH - tw) / 2 , WINDOW_HEIGHT - 50, tw, th };
					SDL_RenderCopy(renderer, tex, nullptr, &dst);
					SDL_DestroyTexture(tex);
				}

				SDL_RenderPresent(renderer); // Muestro imagen renderizada

				// Genero frames estables
				auto stopTime = chrono::high_resolution_clock::now();
				dt = chrono::duration<float,
					chrono::milliseconds::period>(stopTime - startTime).count();
			}
			// — TERMINÓ LA PARTIDA: define resultado
			string msg;
			if (playerOneScore >= maxPoints) {
				msg = "Jugador 1 GANA";
				saveRecord("historialPong", playerOneScore, cpuScore);
				SDL_RenderClear(renderer); // Limpio ventana antes de renderizar
				SDL_RenderCopy(renderer, gameBgTex, nullptr, nullptr);
				showEndMatch(renderer, resultFont, menuFont, msg);
			}
			else if (cpuScore >= maxPoints) {
				msg = "CPU GANA";
				saveRecord("historialPong", playerOneScore, cpuScore);
				SDL_RenderClear(renderer); // Limpio ventana antes de renderizar
				SDL_RenderCopy(renderer, gameBgTex, nullptr, nullptr);
				showEndMatch(renderer, resultFont, menuFont, msg);
			}
			else if (timeRemain <= 0.0f) {
				BALL_SPEED = OG_BALLSPEED;
				if (playerOneScore > cpuScore) {
					msg = "JUGADOR 1 GANA";
				}
				else if (cpuScore > playerOneScore) {
					msg = "CPU GANA";
				}
				else {
					msg = "EMPATE";
				}
				saveRecord("historialPong", playerOneScore, cpuScore);
				SDL_RenderClear(renderer); // Limpio ventana antes de renderizar
				SDL_RenderCopy(renderer, gameBgTex, nullptr, nullptr);
				showEndMatch(renderer, resultFont, menuFont, msg);
			}
		}
	}

	Mix_FreeMusic(music);
	Mix_FreeChunk(wallHitSound);
	Mix_FreeChunk(paddleHitSound);
	Mix_CloseAudio();
	SDL_DestroyTexture(gameBgTex);
	SDL_DestroyTexture(menuBgTex);
	SDL_DestroyTexture(paddleSprite);
	SDL_DestroyTexture(ballSprite);
	SDL_DestroyTexture(studioLogo);
	TTF_CloseFont(menuFont);
	TTF_CloseFont(titleFont);
	TTF_CloseFont(resultFont);
	TTF_CloseFont(introFont);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	Mix_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();

	return 0;
}
