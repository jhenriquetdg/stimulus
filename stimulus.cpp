#include <stdlib.h>

#include <format>
#include <iostream>

#include <raylib.h>
#include <functional>
#include <vector>
#include <complex>
#include <math.h>
#include <chrono>

using namespace std;

unsigned int screenFPS = 60;

const int screenWidth = 800;
const int screenHeight = 800;

unsigned int middleXScreen = screenWidth / 2;
unsigned int middleYScreen = screenHeight / 2;

bool shouldBreak = false;
bool isRunning = false;

typedef enum Screen
{
    LOGO = 0,
    TITLE,
    FIXING,
    STIMULUS,
    ENDING
} Screen;

class Person
{
private:
    string name;
    string email;

public:
    string setName(string name)
    {
        this->name = name;
        return this->name;
    }
    string getName()
    {
        return this->name;
    }
};

class Subject : Person
{
};

class Experimenter : Person
{
};

class Stimulus
{
private:
    int fps = 60;
    float duration = 2;
    int repetitions = 5;
    vector<int> keys = {};
    vector<double> timestamps = {};

    int skipKey = KEY_ENTER;
    Color background = RAYWHITE;
public:
    virtual void draw(void) = 0;

    void run()
    {
        isRunning = true;
        int frame_end = (int)(this->duration * this->fps);

        for (int r = 0; r < this->repetitions; r++)
        {
            int frame_count = 0;
            
            auto t_start = chrono::high_resolution_clock::now();
            SetTargetFPS(this->fps);

            while (!shouldBreak && (frame_count < frame_end))
            {
                frame_count++;
                BeginDrawing();
                ClearBackground(this->background);
                this->draw();
                auto t_frame = chrono::high_resolution_clock::now();
                int current_key = GetKeyPressed();

                if (current_key)
                {
                    double timestamp = (t_frame - t_start).count();
                    this->keys.push_back(current_key);
                    this->timestamps.push_back(timestamp);

                    cout << "Frame:    " << frame_count << endl;
                    cout << "Key:      " << current_key << endl;    
                    cout << "Timestamp:" << timestamp << endl;
                    
                    if (current_key == this->skipKey) break;
                }
                EndDrawing();
            }
            ClearBackground(RAYWHITE);
            isRunning = false;
        }
    }
};

class RandomCircles : public Stimulus
{
private:
    int n_elements = 100;

    int inner_radius = 100;
    int outter_radius = 200;
    int radius = 5;
    Color color = BLACK;

public:
    void draw() override
    {
        complex<double> center;
        for (int d = 0; d < this->n_elements; d++)
        {
            double theta = rand() % 360;
            double r = this->inner_radius + rand() % (this->outter_radius - this->inner_radius);
            center = r * exp(1i * theta);
            DrawCircle(real(center) + middleXScreen, imag(center) + middleYScreen, this->radius, this->color);
        }
    }
};

class FixingCenter : public Stimulus
{
private:
    const char* sign = "+";
    int font_size = 10;
    int center_x = middleXScreen;
    int center_y = middleYScreen;
    Color color = LIGHTGRAY;
public:
    void draw() override
    {
        DrawText(this->sign, this->center_x, this->center_y, this->font_size, this->color);
    }
};

using word_color = pair<char*, Color>;
class OddWordColor : public Stimulus 
{
private:
    int font_size = 20;

    vector<word_color> wc = {
        word_color("Gray", GRAY),
        word_color("Yellow", YELLOW),
        word_color("Gold", GOLD),
        word_color("Orange", ORANGE),
        word_color("Pink", PINK),
        word_color("Red", RED),
        word_color("Maroon", MAROON),
        word_color("Green", GREEN),
        word_color("Lime", LIME),
        word_color("Blue", BLUE),
        word_color("Purple", PURPLE),
        word_color("Violet", VIOLET),
        word_color("Beige", BEIGE),
        word_color("Brown", BROWN),
        word_color("White", WHITE),
        word_color("Black", BLACK),
        word_color("Magenta", MAGENTA)
    };
public: 
    void draw() override 
    {
        unsigned int word_index = rand() % wc.size();
        unsigned int color_index = rand() % wc.size();
        DrawText(this->wc[word_index].first, middleXScreen, middleYScreen, this->font_size, this->wc[color_index].second);
    }

};

class Experiment
{
private:
    string name;
    vector<Stimulus> batch;
};

int main(void)
{
    // Setting raylib variables

    InitWindow(screenWidth, screenHeight, "RayPort Sampler");

    SetTargetFPS(screenFPS);

    Screen currentScreen = LOGO;

    // Setting portaudio variables
    // Initializing

    bool shouldClose = false;

    unsigned int logoTime = 2;  // in seconds
    unsigned int titleTime = 5; // in seconds

    unsigned int skipCount = 0;
    unsigned int frameCount = 0;

    double dt = 1 / screenFPS;
    
    while (!shouldClose)
    {

        cout << "Frame Count" << frameCount << endl;
        frameCount++;

        shouldClose = IsKeyPressed(KEY_ESCAPE) || WindowShouldClose();

        switch (currentScreen)
        {
        case LOGO:
            skipCount++;
            if (IsKeyPressed(KEY_ENTER) || skipCount > logoTime * screenFPS)
            {
                currentScreen = TITLE;
                skipCount = 0;
            }
            break;
        case TITLE:
            skipCount++;
            if (IsKeyPressed(KEY_ENTER) || skipCount > titleTime * screenFPS)
            {
                currentScreen = LOGO;
                skipCount = 0;
            }
            break;
        default:
            break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawFPS(40,40);
        
        switch (currentScreen)
        {
        case LOGO:
            DrawText("S", 10, middleYScreen, 50, LIGHTGRAY);
            break;

        case TITLE:
            if (!isRunning){
                OddWordColor().run();
                // FixingCenter().run();
                // RandomCircles().run();
                currentScreen = LOGO;
            }
            break;

        default:
            break;
        }

        string content;

        DrawText(TextFormat("A label %d text", frameCount), 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    // Releasing raylib window

    CloseWindow();

    return EXIT_SUCCESS;
}