/*
 * Developed as part of undergraduate course in information technology.
 *
 *
 *
 */

#define RAYLIB_IMPLEMENTATION
#include <./raylib.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <complex>
#include <math.h>
#include <chrono>

#include <tuple>

using namespace std;

unsigned int screen_FPS = 60;

const int screen_width = 800;
const int screen_height = 800;

unsigned int middle_x_screen = screen_width / 2;
unsigned int middle_y_screen = screen_height / 2;

bool should_break = false;
bool is_running = false;
bool is_editting = true;

typedef enum Screen
{
    LOGO = 0,
    MAIN,
    EDITTING,
    RUNNING,
    ENDING
} Screen;

typedef enum Stim
{
    FIXING,
    RANDOM_CIRCLE,
    COLORED_WORDS,
} Stim;

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
    int FPS = 160;
    int duration = 20;
    int repetitions = 1;
    int random_seed = 0;

    vector<int> keys = {};
    vector<double> timestamps = {};

    int skip_key = KEY_ENTER;
    Color background = RAYWHITE;

public:
    bool pick_once = false;

    int *get_ref_FPS() { return &this->FPS; }
    int *get_ref_duration() { return &this->duration; }
    int *get_ref_repetitions() { return &this->repetitions; }
    int *get_ref_random_seeds() { return &this->random_seed; }

    virtual void draw(void) = 0;
    virtual void pick(void) = 0;
    void run()
    {
        is_running = true;
        int frame_end = (int)(this->duration * this->FPS);

        SetTargetFPS(this->FPS);

        srand(this->random_seed);
        for (int r = 0; r <= this->repetitions; r++)
        {
            int frame_count = 0;

            auto t_start = chrono::high_resolution_clock::now();

            this->pick();
            while (!should_break && (frame_count < frame_end))
            {
                frame_count++;
                BeginDrawing();
                ClearBackground(this->background);
                if (!this->pick_once)
                {
                    this->pick();
                }
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

                    if (current_key == this->skip_key)
                        should_break = true;
                }
                EndDrawing();
            }
            ClearBackground(RAYWHITE);
            is_running = false;
        }
    }
};

class RandomCircles : public Stimulus
{
private:
    int n = 100;  // number of elements
    int size = 5; // shape size

    int inner_radius = 100;  // inner radius
    int outter_radius = 200; // outter radius

    complex<double> *points = new complex<double>[100];

    Color color = BLACK;

public:
    RandomCircles() {}
    RandomCircles(int n) : n(n) {}
    RandomCircles(int n, int s) : n(n), size(s) {}

    int *get_ref_n() { return &this->n; }
    int *get_ref_size() { return &this->size; }
    int *get_ref_inner_radius() { return &this->inner_radius; }
    int *get_ref_outter_radius() { return &this->outter_radius; }

    void pick() override
    {
        this->points = new complex<double>[this->n];
        for (int p = 0; p < this->n; p++)
        {
            double r = 0;
            double theta = rand() % 360;

            int shift = this->outter_radius - this->inner_radius;

            if (shift <= 0)
                r = this->inner_radius;
            else
                r = this->inner_radius + rand() % shift;
            this->points[p] = r * exp(1i * theta);
        }
    }
    void draw() override
    {
        for (int p = 0; p < this->n; p++)
        {
            DrawCircle(real(this->points[p]) + middle_x_screen, imag(this->points[p]) + middle_y_screen, this->size, this->color);
        }
    }
};

class FixingCenter : public Stimulus
{
private:
    const char *sign = "+";
    int font_size = 10;
    int center_x = middle_x_screen;
    int center_y = middle_y_screen;
    Color color = LIGHTGRAY;

public:
    void draw() override
    {
        DrawText(this->sign, this->center_x, this->center_y, this->font_size, this->color);
    }
};

using word_color = pair<char const*, Color>;
class ColoredWords : public Stimulus
{
private:
    int font_size = 20;
    int word_index;
    int color_index;

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
        word_color("Magenta", MAGENTA)};

public:
    void pick()
    {
        word_index = rand() % wc.size();
        color_index = rand() % wc.size();
    }
    void draw() override
    {
        DrawText(wc[word_index].first, middle_x_screen, middle_y_screen, font_size, wc[color_index].second);
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
    InitWindow(screen_width, screen_height, "RayPort Sampler");
    SetTargetFPS(screen_FPS);

    Screen current_screen = LOGO;

    vector<Stimulus *> stimuli = {};

    bool shouldClose = false;

    unsigned int logo_time = 5;    // in seconds
    unsigned int title_time = 100; // in seconds

    bool is_escaping = false;
    unsigned int escape_time = 5;
    unsigned int escape_count = 0;

    unsigned int skip_count = 0;

    unsigned int frame_count = 0;

    double dt = 1 / screen_FPS;

    int font_size = 20;
    int is_show_FPS = false;

    while (!shouldClose)
    {
        frame_count++;
        GuiSetStyle(DEFAULT, TEXT_SIZE, font_size);
        SetExitKey(KEY_NULL);
        shouldClose = WindowShouldClose();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        if (is_show_FPS)
            DrawFPS(40, 40);
        if (IsKeyPressed(KEY_F))
        {
            is_show_FPS = !is_show_FPS;
        }

        switch (current_screen)
        {
        case LOGO:
            DrawText("Stimuli", 5, screen_height - 50, 50, LIGHTGRAY);
            skip_count++;
            if (IsKeyPressed(KEY_ENTER) || skip_count > logo_time * screen_FPS)
            {
                current_screen = MAIN;
                skip_count = 0;
            }
            break;

        case MAIN:
            DrawText("Main", 5, screen_height - 50, 50, LIGHTGRAY);
            skip_count++;

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E))
            {
                is_editting = true;
                is_running = false;
                current_screen = EDITTING;
            }
            break;

        case EDITTING:
            if (!is_running)
            {
                RandomCircles *rc = new RandomCircles(1000, 2);
                int f_current = 0;
                bool show_FPS = true;
                rc->pick();
                while (is_editting)
                {
                    BeginDrawing();
                    ClearBackground(RAYWHITE);
                    SetTargetFPS(*rc->get_ref_FPS());

                    if (show_FPS)
                        DrawFPS(10, 10);
                    if (!rc->pick_once)
                        rc->pick();
                    rc->draw();
                    using field = tuple<const char *, int *, int, int>;
                    vector<field> field_vector = {
                        make_tuple("N", rc->get_ref_n(), 1, 1000),
                        make_tuple("size", rc->get_ref_size(), 1, 1000),
                        make_tuple("inner", rc->get_ref_inner_radius(), 1, 1000),
                        make_tuple("outter", rc->get_ref_outter_radius(), 1, 1000),
                        make_tuple("FPS", rc->get_ref_FPS(), 1, 1000),
                        make_tuple("duration", rc->get_ref_duration(), 1, 1000),
                        make_tuple("seed", rc->get_ref_random_seeds(), 1, 1000),
                    };

                    float field_height = 0;
                    int field_count = 0;

                    int field_index = f_current % field_vector.size();

                    for (field f : field_vector)
                    {
                        GuiValueBox(
                            (Rectangle){600, 140 + field_height, 120, 20},
                            get<0>(f),
                            get<1>(f),
                            get<2>(f),
                            get<3>(f),
                            field_index == field_count);
                        field_count += 1;
                        field_height += 25;
                    }

                    if (IsKeyPressed(KEY_UP) || IsKeyDown(KEY_RIGHT))
                    {
                        if (*get<1>(field_vector[field_index]) < get<3>(field_vector[field_index]))
                            *get<1>(field_vector[field_index]) += 1;
                    }

                    if (IsKeyPressed(KEY_DOWN) || IsKeyDown(KEY_LEFT))
                    {
                        if (*get<1>(field_vector[field_index]) > get<2>(field_vector[field_index]))
                            *get<1>(field_vector[field_index]) -= 1;
                    }

                    if (IsKeyPressed(KEY_TAB))
                    {
                        if (IsKeyDown(KEY_LEFT_SHIFT))
                        {
                            f_current--;
                        }
                        else
                        {
                            f_current++;
                        }
                    }

                    if (IsKeyPressed(KEY_S))
                    {
                        if (IsKeyDown(KEY_LEFT_CONTROL))
                        {
                            stimuli.push_back(rc);
                            is_editting = false;
                        }
                    }

                    if (IsKeyPressed(KEY_C))
                    {
                        if (IsKeyDown(KEY_LEFT_CONTROL))
                        {
                            delete rc;
                            is_editting = false;
                        }
                    }

                    if (IsKeyPressed(KEY_F))
                    {
                        show_FPS = !show_FPS;
                    }

                    EndDrawing();
                }

                current_screen = MAIN;
            }
            break;

        default:
            break;
        }
        // DrawText(TextFormat("A label %d text", frameCount), 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
