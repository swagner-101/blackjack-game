//Using SDL, SDL_image, standard IO, and strings
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <SDL2_mixer/SDL_mixer.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <unistd.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int CARD_WIDTH = 112;
const int CARD_LENGTH = 156;

const int TOTAL_CARDS = 52;
//Globally used font
TTF_Font* gFont = NULL;
//declare functions
//Starts up SDL and creates window
bool init();

//Frees media and shuts down SDL
void close();

//Loads media
bool loadMedia();

//declare windows and renderer
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Rect gCardClips[ TOTAL_CARDS ];

//The music that will be played
Mix_Music *gMusic = NULL;


///////////////////////////////////////////texture class and class functions/////////////////////////////////


//Texture wrapper class
class LTexture
{
    public:
        //Initializes variables
        LTexture();
    
        //Deallocates memory
        ~LTexture();
    
        //Loads image at specified path
        bool loadFromFile( std::string path );
    
    #ifdef _SDL_TTF_H
        //Creates image from font string
        bool loadFromRenderedText( std::string textureText, SDL_Color textColor );
    #endif
    
        //Deallocates texture
        void free();
    
        //Renders texture at given point
        void render( int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE );
    
        //Gets image dimensions
        int getWidth();
        int getHeight();
    
    private:
        //The actual hardware texture
        SDL_Texture* mTexture;
    
        //Image dimensions
        int mWidth;
        int mHeight;
};

bool LTexture::loadFromFile( std::string path )
{
    //Get rid of preexisting texture
    free();
    
    //The final texture
    SDL_Texture* newTexture = NULL;
    
    //Load image at specified path
    SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
    if( loadedSurface == NULL )
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
    }
    else
    {
        //Color key image
        SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );
        
        //Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
        if( newTexture == NULL )
        {
            printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
        }
        else
        {
            //Get image dimensions
            mWidth = loadedSurface->w;
            mHeight = loadedSurface->h;
        }
        
        //Get rid of old loaded surface
        SDL_FreeSurface( loadedSurface );
    }
    
    //Return success
    mTexture = newTexture;
    return mTexture != NULL;
}

bool LTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor )
{
    //Get rid of preexisting texture
    free();
    
    //Render text surface
    SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, textureText.c_str(), textColor );
    if( textSurface == NULL )
    {
        printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
    }
    else
    {
        //Create texture from surface pixels
        mTexture = SDL_CreateTextureFromSurface( gRenderer, textSurface );
        if( mTexture == NULL )
        {
            printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
        }
        else
        {
            //Get image dimensions
            mWidth = textSurface->w;
            mHeight = textSurface->h;
        }
        
        //Get rid of old surface
        SDL_FreeSurface( textSurface );
    }
    
    //Return success
    return mTexture != NULL;
}

void LTexture::render( int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip )
{
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };
    
    //Set clip rendering dimensions
    if( clip != NULL )
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    
    //Render to screen
    SDL_RenderCopyEx( gRenderer, mTexture, clip, &renderQuad, angle, center, flip );
}

LTexture::LTexture()
{
    //Initialize
    mTexture = NULL;
    mWidth = 0;
    mHeight = 0;
}

LTexture::~LTexture()
{
    //Deallocate
    free();
}

int LTexture::getWidth()
{
    return mWidth;
}

int LTexture::getHeight()
{
    return mHeight;
}

void LTexture::free()
{
    //Free texture if it exists
    if( mTexture != NULL )
    {
        SDL_DestroyTexture( mTexture );
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}
///////////////////////////declare card class and functions//////////////////////////////////////////

class LCard
{
    public:
        //initializes card
        LCard();
    
        //returns value of card
        int getValue();
    
        //displays card
        void render();
    
    private:
        int value;
    
};

LCard::LCard()
{
    value = 0;
}

int LCard::getValue()
{
    return value;
}



///////////////////////////media load/ close/ init / main/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

LTexture top_deck;
LTexture blue_chip;
LTexture grey_chip;
LTexture white_chip;
LTexture cardsTexture;
LTexture TOTAL;

int value_array[TOTAL_CARDS];

void close()
{
    
    top_deck.free();
    blue_chip.free();
    grey_chip.free();
    white_chip.free();
    cardsTexture.free();
    TOTAL.free();
    
    //Destroy window
    SDL_DestroyRenderer( gRenderer );
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;
    gRenderer = NULL;
    
    //Free global font
    TTF_CloseFont( gFont );
    gFont = NULL;
    
    //Free the music
    Mix_FreeMusic( gMusic );
    gMusic = NULL;
    
    //Quit SDL subsystems
    IMG_Quit();
    SDL_Quit();
}

bool init()
{
    //Initialization flag
    bool success = true;
    
    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Set texture filtering to linear
        if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
        {
            printf( "Warning: Linear texture filtering not enabled!" );
        }
        
        //Create window
        gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
        if( gWindow == NULL )
        {
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        }
        else
        {
            //Create vsynced renderer for window
            gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
            if( gRenderer == NULL )
            {
                printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            }
            else
            {
                //Initialize renderer color
                SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
                
                //Initialize PNG loading
                int imgFlags = IMG_INIT_PNG;
                if( !( IMG_Init( imgFlags ) & imgFlags ) )
                {
                    printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
                    success = false;
                }
                
                //Initialize SDL_mixer
                if( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2,  2048) < 0 )
                {
                    printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
                    success = false;
                }
                //Initialize SDL_ttf
                if( TTF_Init() == -1 )
                {
                    printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
                    success = false;
                }
            }
        }
    }
    
    return success;
}

bool loadMedia()
{
    bool success = true;
    
    //Load sprites
    if( !top_deck.loadFromFile( "poker/back.png" ) )
    {
        printf( "Failed to load button sprite texture!\n" );
        success = false;
    }
    
    //Load sprites
    if( !blue_chip.loadFromFile( "poker/chip_blue_top.png" ) )
    {
        printf( "Failed to load button sprite texture!\n" );
        success = false;
    }
    
    //Load sprites
    if( !grey_chip.loadFromFile( "poker/chip_gray_top.png" ) )
    {
        printf( "Failed to load button sprite texture!\n" );
        success = false;
    }
    
    //Load sprites
    if( !white_chip.loadFromFile( "poker/chip_white_top.png" ) )
    {
        printf( "Failed to load button sprite texture!\n" );
        success = false;
    }
    
    if( !cardsTexture.loadFromFile( "poker/cards.png" ) )
    {
        printf( "Failed to load button sprite texture!\n" );
        success = false;
    }
    
    //Open the font
    gFont = TTF_OpenFont( "poker/lazy.ttf", 28 );
    if( gFont == NULL )
    {
        printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
        success = false;
    }

    int j = 0;
    int curr_x=0;
    int curr_y = 0;
    int curr_value = 1;
    for( int i = 0; i < TOTAL_CARDS; ++i )
    {
        j++;
        gCardClips[ i ].x = curr_x;
        gCardClips[ i ].y = curr_y;
        gCardClips[ i ].w = CARD_WIDTH;
        gCardClips[ i ].h = CARD_LENGTH;
        curr_y += CARD_LENGTH;
        
        value_array[i] = curr_value;
        
        if(j == 4){
            if (curr_value != 10){curr_value++;}
            curr_y = 0;
            j = 0;
            curr_x += CARD_WIDTH;
            
        }
    
        
    }
    
    
    //Load music
    gMusic = Mix_LoadMUS( "poker/star wars cantina.wav" );
    if( gMusic == NULL )
    {
        printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }
    
    return success;
}

void shuffle(){
    
    
    SDL_Rect temprect;
    int tempint;
    int swap;
    
    for(int i = 0; i<51; i++){
        //generate rand int for remaining
        swap = rand() % (52-i) + i;
        
        //swap rectangles in array
        temprect = gCardClips[i];
        gCardClips[i] = gCardClips[swap];
        gCardClips[swap] = temprect;
        
        //swap same values in int array
        tempint = value_array[i];
        value_array[i] = value_array[swap];
        value_array[swap] = tempint;
        
    }
    
    
}

int main( int argc, char* args[] )
{
    //Start up SDL and create window
    if( !init() )
    {
        printf( "Failed to initialize!\n" );
    }
    else
    {
        //Load media
        if( !loadMedia() )
        {
            printf( "Failed to load media!\n" );
        }
        else
        {
            //Main loop flag
            bool quit = false;
            
            //Event handler
            SDL_Event e;
            
            //Play the music
            Mix_PlayMusic( gMusic, -1 );
            
            //Set text color as black
            SDL_Color textColor = { 102, 0, 204, 255 };
            
            //In memory text stream
            std::stringstream totaltext;
        
            shuffle();
            bool phase2 = false;
            bool phase1 = false;
            int count = 2;
            
            //While application is running
            while( !quit )
            {
                //Handle events on queue
                while( SDL_PollEvent( &e ) != 0 )
                {
                    //User requests quit
                    if( e.type == SDL_QUIT )
                    {
                        quit = true;
                    }
                    else if(e.type == SDL_KEYDOWN)
                    {
                        switch( e.key.keysym.sym )
                        {
                            case SDLK_d:
                                phase1 = true;
                                count = 2;
                                shuffle();
                                break;
                            case SDLK_h:
                                count++;
                                break;
                            case SDLK_s:
                                phase2 = true;
                                break;
                                
                        }
                    }
                  
                }
                
                
                //Clear screen
                SDL_SetRenderDrawColor( gRenderer, 0, 0xCC, 0, 0xFF );
                SDL_RenderClear( gRenderer );
                
                top_deck.render(20,20);
                grey_chip.render(SCREEN_WIDTH/2, SCREEN_HEIGHT-100);
                blue_chip.render(SCREEN_WIDTH/2+80, SCREEN_HEIGHT-100);
                white_chip.render(SCREEN_WIDTH/2+160, SCREEN_HEIGHT-100);
                int total = 0;
                
                if(phase1){
                    //render players cards
                    int positionx =SCREEN_HEIGHT/4;
                    int positiony =SCREEN_HEIGHT/2-40;
                    for(int i = 0; i < count; i++){
                        cardsTexture.render(positionx, positiony,&gCardClips[i]);
                        positionx += 56;
                        if (value_array[i] == 1 && (11 + total)<=21 ){
                            total = total + 11;}
                        else{total = total + value_array[i];}
                        
                    }
                
                    totaltext.str( "" );
                    totaltext << " Total : " << total;
                
                    //Render text
                    if( !TOTAL.loadFromRenderedText( totaltext.str().c_str(), textColor ) )
                    {
                        printf( "Unable to render FPS texture!\n" );
                    }
                    
                    TOTAL.render(80, SCREEN_HEIGHT-100);
                    
                    if (!phase2){
                        top_deck.render(SCREEN_HEIGHT/2 + 56, 20);}
                    
                    cardsTexture.render(SCREEN_HEIGHT/2, 20,&gCardClips[51]);
                    
                    
                
                    if(total > 21){
                        if( !TOTAL.loadFromRenderedText( "DEALER WINS press d to deal again", textColor ) )
                        {
                            printf( "Unable to render FPS texture!\n" );
                        }
                        TOTAL.render(90, SCREEN_HEIGHT/2);
                        SDL_RenderPresent( gRenderer );
                    
                        usleep(3000000);
                        phase1 = false;
                        phase2 = false;
                        
                    }
                }//end phase 1
                
                if (phase2){
                    int dtotal = value_array[51];
                    
                    int Opositionx =SCREEN_HEIGHT/2+56;
                    int Opositiony =20;
                    for(int k=50; dtotal<= total; k--){
                        cardsTexture.render(Opositionx, Opositiony,&gCardClips[k]);
                        dtotal = dtotal + value_array[k];
                        Opositionx += 56;
                    }
                    
                    if (dtotal > total && dtotal<=21){
                        if( !TOTAL.loadFromRenderedText( "DEALER WINS press d to deal again", textColor ) )
                        {
                            printf( "Unable to render FPS texture!\n" );
                        }
                        TOTAL.render(90, SCREEN_HEIGHT/2);
                        SDL_RenderPresent( gRenderer );
                        
                        usleep(3000000);
                        phase1 = false;
                        phase2 = false;
                    }
                    
                    if (dtotal == total){
                        if( !TOTAL.loadFromRenderedText( "PUSH press d to deal again", textColor ) )
                        {
                            printf( "Unable to render FPS texture!\n" );
                        }
                        TOTAL.render(90, SCREEN_HEIGHT/2);
                        SDL_RenderPresent( gRenderer );
                        
                        usleep(3000000);
                        phase1 = false;
                        phase2 = false;
                    }
                    
                    if (dtotal > 21){
                        if( !TOTAL.loadFromRenderedText( "YOU WIN press d to deal again", textColor ) )
                        {
                            printf( "Unable to render FPS texture!\n" );
                        }
                        TOTAL.render(90, SCREEN_HEIGHT/2);
                        SDL_RenderPresent( gRenderer );
                        
                        usleep(3000000);
                        phase1 = false;
                        phase2 = false;
                    }
                    
                        
                    
                    
                }
        
                //Update screen
                SDL_RenderPresent( gRenderer );
            }
        }
    }
    
    //Free resources and close SDL
    close();
    
    return 0;
}
