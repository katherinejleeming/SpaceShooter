#define PLAY_IMPLEMENTATION
#include "Play.h"

const int DISPLAY_WIDTH = 1280;
const int DISPLAY_HEIGHT = 720;
const int DISPLAY_SCALE = 1;

const Point2f PLAYER_START_POS = { 640, 650 };
const float PLAYER_SPEED = 5.0f;
const Point2f SAUCER_START_POS = { 1500, 125 };
const int SAUCER_SCORE = 1000;
const float SAUCER_SPEED_INCREMENT = 5.0f;
const float LASER_SPEED = 20.0f;
const int LASER_COST = 100;
constexpr int MAX_LASERS = 100;
constexpr int MAX_SAUCERS = 5;


struct Laser
{
    Point2f laserPos{ 0, 0 };
    Vector2f velocity{ 0,0 };
};


struct Saucer
{
    Point2f saucerPos = SAUCER_START_POS;
    float saucerRot{ 0 };
    float saucerSpeed = SAUCER_SPEED_INCREMENT;
    bool saucerIsDead{ false };
    bool active{ false };
    Vector2f velocity{ 0,0 };
};

struct GameState
{
    float time{ 0 };
    float saucerSpawn{ 0 };
    int score{ 0 };
    Point2f playerPos = PLAYER_START_POS;
    Laser lasers[MAX_LASERS];
    Saucer saucers[MAX_SAUCERS];
};

GameState gState;

void UpdatePlayer();
void UpdateLaser();
void UpdateSaucer();
bool HasCollided( Point2f pos1, Point2f pos2 );

// The entry point for a Windows program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
    Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
    Play::CentreAllSpriteOrigins();
    Play::LoadBackground( "Data\\Backgrounds\\background.png" );
    //Play::StartAudioLoop( "music" );
}

// Called by the PlayBuffer once for each frame of the game (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
    gState.time += elapsedTime;
    gState.saucerSpawn += elapsedTime;

    if (gState.saucerSpawn > 1.0f)
    {
        gState.saucerSpawn = 0.0f;
        for (int n = 0; n < MAX_SAUCERS; n++)
        {
            Saucer& s = gState.saucers[n];
            if (s.active == false)
            {
                s.active = true;
                s.saucerPos = SAUCER_START_POS;
                s.saucerPos.y = Play::RandomRollRange(-20, 300);
                s.saucerRot = { 0 };
                s.saucerSpeed = SAUCER_SPEED_INCREMENT;
                s.saucerIsDead = { false };
                s.velocity = { 0,0 };
                break;
            }
        }
    }

    Play::DrawBackground();
    UpdateLaser();
    UpdatePlayer();
    UpdateSaucer();
    Play::DrawFontText( "105px", "SCORE: " + std::to_string( gState.score ),
        { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE );
    Play::PresentDrawingBuffer();
    return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
    Play::DestroyManager();
    return PLAY_OK;
}

void UpdatePlayer( void )
{

    if( Play::KeyDown( VK_LEFT ) )
        gState.playerPos.x -= PLAYER_SPEED;

    if( Play::KeyDown( VK_RIGHT ) )
        gState.playerPos.x += PLAYER_SPEED;

    if( Play::KeyPressed( VK_SPACE ) )
    {
        for (int n = 0; n < MAX_LASERS; n++)
        {
            if (gState.lasers[n].laserPos.y <= 0)
            {
                gState.lasers[n].laserPos.x = gState.playerPos.x;
                gState.lasers[n].laserPos.y = gState.playerPos.y - 50;
                if (gState.score >= LASER_COST)
                    gState.score -= LASER_COST;
                Play::PlayAudio("laser");
                break;
            }
        }
    }

    float yWobble = sin( gState.time * PLAY_PI ) * 3;
    Play::DrawSprite( Play::GetSpriteId( "Rocket" ), { gState.playerPos.x, gState.playerPos.y + yWobble }, (int)( 2.0f * gState.time ) );
}

void UpdateLaser( void )
{
    //for (int i = 0; i < MAX_LASERS; i++)
    for (Laser& l : gState.lasers)
    {

        //Laser& l = gState.lasers[i];
        if (l.laserPos.y > 0.0f)
        {

            l.laserPos.y -= LASER_SPEED;
            for (int j = 0; j < MAX_SAUCERS; j++)
            {
                Saucer& s = gState.saucers[j];
                if (s.active == true)
                {
                    if (HasCollided(l.laserPos, s.saucerPos))
                    {
                        l.laserPos.y = -100; // Hide the laser off screen!
                        s.saucerIsDead = true;
                        gState.score += SAUCER_SCORE;
                        s.saucerSpeed += SAUCER_SPEED_INCREMENT;
                        Play::PlayAudio("clang");
                    }
                }
            }    
            Play::DrawSprite(Play::GetSpriteId("Laser"), l.laserPos, 0);
        }
    }
}

void UpdateSaucer( void )
{
    const int OFF_SCREEN_TEST = -50;

    for (int n = 0; n < MAX_SAUCERS; n++)
    {
        Saucer& s = gState.saucers[n];
        if (s.active == true)
        {
            //s.saucerPos += s.velocity;

            if (!s.saucerIsDead)
            {
                s.saucerPos.x -= s.saucerSpeed;
                s.saucerPos.y += sin(s.saucerPos.x / 100) * 3;

                if (s.saucerPos.x < OFF_SCREEN_TEST)
                    s.saucerPos = SAUCER_START_POS;
            }
            else
            {
                s.saucerPos.x -= s.saucerSpeed;
                s.saucerPos.y += 2;
                s.saucerRot += 0.01f * s.saucerSpeed;

                if (s.saucerPos.x < OFF_SCREEN_TEST)
                {
                    s.active = false;
                }
            }

            Play::DrawSpriteRotated(Play::GetSpriteId("Saucer"), s.saucerPos, 0, s.saucerRot, 1.0f);
        }
    }
}

bool HasCollided( Point2f pos1, Point2f pos2 )
{
    const float DISTANCE_TEST = 50.0f;

    Vector2f separation = pos2 - pos1;
    float dist = sqrt( ( separation.x * separation.x ) + ( separation.y * separation.y ) );
    if( dist < DISTANCE_TEST )
        return true;
    else
        return false;
}



