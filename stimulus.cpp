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

#include <jsoncpp/json/json.h>

#include <sqlite3.h>

#include <stdlib.h>
#include <iostream>
#include <complex>
#include <vector>
#include <math.h>
#include <chrono>
#include <tuple>
#include <fstream>
#include <filesystem>

using namespace std;

unsigned int screen_FPS = 60;

const int screen_width = 800;
const int screen_height = 800;

unsigned int middle_x_screen = screen_width / 2;
unsigned int middle_y_screen = screen_height / 2;

bool should_break = false;
bool is_presenting = false;
bool is_editting = true;

typedef enum Screen
{
    LOGO = 0,
    MAIN,
    EDITTING,
    PRESENTING,
    REPORT,
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
public:
    int FPS = 60;
    int duration = 10;
    int repetitions = 1;
    int random_seed = 0;

    vector<int> keys = {};
    vector<double> timestamps = {};

    int skip_key = KEY_ENTER;
    Color background = RAYWHITE;

    bool pick_once = false;

    virtual void pick(void) = 0;
    virtual void draw(void) = 0;
    virtual Json::String to_json(void) = 0;

    void save()
    {
        stringstream stream;

        stream << hex << filesystem::hash_value(this->to_json());

        string filename(stream.str());

        ofstream file = ofstream("./files/stimuli/" + filename + ".json", ios::out);

        file << this->to_json();

        file.close();
    }

    void present()
    {
        is_presenting = true;
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
            is_presenting = false;
        }
    }
};

class RandomCircles : public Stimulus
{
public:
    int n = 100;  // number of elements
    int size = 5; // shape size

    int inner_radius = 100;  // inner radius
    int outter_radius = 200; // outter radius

    complex<double> *points = new complex<double>[100];

    Color color = BLACK;

    RandomCircles() {}
    RandomCircles(int n, int s, int irad, int orad, int FPS, int duration, int repetitions, int random_seed)
    {
        this->n = n;
        this->size = s;
        this->inner_radius = irad;
        this->outter_radius = orad;

        this->FPS = FPS;
        this->duration = duration;
        this->repetitions = repetitions;
        this->random_seed = random_seed;
    }
    ~RandomCircles()
    {
        delete[] points;
    }

    void pick() override
    {
        this->points = new complex<double>[this->n];
        for (int p = 0; p < this->n; p++)
        {
            double r = 0;
            double theta = rand() % 360;

            int diff_radius = this->outter_radius - this->inner_radius;

            if (diff_radius <= 0)
                r = this->inner_radius;
            else
                r = this->inner_radius + rand() % diff_radius;

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

    Json::String to_json()
    {
        Json::Value root;

        root["type"] = "RandomCircles";
        root["n"] = this->n;
        root["size"] = this->size;
        root["inner_radius"] = this->inner_radius;
        root["outter_radius"] = this->outter_radius;
        root["FPS"] = this->FPS;
        root["duration"] = this->duration;
        root["repetitions"] = this->repetitions;
        root["random_seed"] = this->random_seed;

        return root.toStyledString();
    }

    static RandomCircles from_json(Json::Value root)
    {
        RandomCircles *rc = NULL;

        if (root["type"] != "RandomCircles")
        {
            rc->n = root.isMember("n") ? root["n"].asInt() : 100;
            rc->size = root.isMember("size") ? root["size"].asInt() : 5;
            rc->inner_radius = root.isMember("inner_radius") ? root["inner_radius"].asInt() : 100;
            rc->outter_radius = root.isMember("outter_radius") ? root["outter_radius"].asInt() : 120;
            rc->FPS = root.isMember("FPS") ? root["FPS"].asInt() : 60;
            rc->duration = root.isMember("duration") ? root["duration"].asInt() : 30;
            rc->repetitions = root.isMember("repetitions") ? root["repetitions"].asInt() : 1;
            rc->random_seed = root.isMember("random_seed") ? root["random_seed"].asInt() : 0;
            rc->pick_once = false;
        }
        else
        {
            cerr << "Failed to load file; incorrect type." << endl;
        }

        return *rc;
    }
};

class Fixing : public Stimulus
{
private:
    const char *sign = "+";
    int font_size = 10;
    int center_x = middle_x_screen;
    int center_y = middle_y_screen;
    Color color = LIGHTGRAY;

public:
    void pick() override
    {
    }

    void draw() override
    {
        DrawText(this->sign, this->center_x, this->center_y, this->font_size, this->color);
    }

    Json::String to_json()
    {
        Json::Value root;

        root["type"] = "Fixing";
        root["sign"] = this->sign;
        root["font_size"] = this->font_size;
        root["center_x"] = this->center_x;
        root["center_y"] = this->center_y;
        root["FPS"] = this->FPS;
        root["duration"] = this->duration;
        root["repetitions"] = this->repetitions;
        root["random_seed"] = this->random_seed;

        return root.toStyledString();
    }
    static Fixing from_json(Json::Value root)
    {
        Fixing *s = NULL;

        if (root["type"] != "Fixing")
        {
            s->sign = root.isMember("sign") ? root["sign"].asCString() : "+";
            s->font_size = root.isMember("font_size") ? root["font_size"].asInt() : 1;
            s->center_x = root.isMember("center_x") ? root["center_x"].asInt() : 1;
            s->center_y = root.isMember("center_Y") ? root["center_Y"].asInt() : 1;
            s->FPS = root.isMember("FPS") ? root["FPS"].asInt() : 60;
            s->duration = root.isMember("duration") ? root["duration"].asInt() : 5;
            s->repetitions = root.isMember("repetitions") ? root["repetitions"].asInt() : 1;
            s->random_seed = root.isMember("random_seed") ? root["random_seed"].asInt() : 0;
            s->pick_once = true;
        }
        else
        {
            cerr << "Failed to load file; incorrect type." << endl;
        }

        return *s;
    }
};

using word_color = pair<char const *, Color>;
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

    Json::String to_json()
    {
        Json::Value root;

        root["type"] = "ColoredWords";
        root["font_size"] = this->font_size;
        root["FPS"] = this->FPS;
        root["duration"] = this->duration;
        root["repetitionw"] = this->repetitions;
        root["random_seed"] = this->random_seed;

        return root.toStyledString();
    }
    static ColoredWords from_json(Json::Value root)
    {
        ColoredWords *s = NULL;

        if (root["type"] != "ColoredWords")
        {
            s->font_size = root.isMember("font_size") ? root["font_size"].asInt() : 20;
            s->FPS = root.isMember("FPS") ? root["FPS"].asInt() : 60;
            s->duration = root.isMember("duration") ? root["duration"].asInt() : 30;
            s->repetitions = root.isMember("repetition") ? root["repetition"].asInt() : 1;
            s->random_seed = root.isMember("random_seed") ? root["random_seed"].asInt() : 0;
            s->pick_once = true;
        }
        else
        {
            cerr << "Failed to load file; incorrect type." << endl;
        }

        return *s;
    }
};

class Experiment
{
private:
    string name;
    vector<Stimulus> batch;

public:
    void to_json()
    {

        // abre arquivo

        // manipula arquivo

        // salva arquivo
    }
};

int main(void)
{
    // Setting raylib variables
    InitWindow(screen_width, screen_height, "Stimulus");
    SetTargetFPS(screen_FPS);

    filesystem::create_directories("./files");
    filesystem::create_directories("./files/stimuli");
    filesystem::create_directories("./files/experiments");
    filesystem::create_directories("./files/people");

    Screen current_screen = LOGO;

    vector<Stimulus *> stimuli = {};

    bool should_close = false;

    unsigned int logo_time = 5; // in seconds

    bool is_escaping = false;
    unsigned int escape_time = 5;
    unsigned int escape_count = 0;

    unsigned int skip_count = 0;

    unsigned int frame_count = 0;

    double dt = 1 / screen_FPS;

    int font_size = 20;
    bool is_show_FPS = false;

    while (!should_close)
    {
        frame_count++;
        GuiSetStyle(DEFAULT, TEXT_SIZE, font_size);
        SetExitKey(KEY_NULL);
        should_close = WindowShouldClose();

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

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D))
            {
                for (auto const &dir_entry : std::filesystem::directory_iterator{"files/stimuli"})
                {
                    ifstream input_file(dir_entry.path());
                    Json::Value root;
                    input_file >> root;
                    input_file.close();

                    if (root.isMember("type"))
                    {
                        if (root["type"] == "Fixing")
                        {
                            Fixing f = Fixing::from_json(root);
                            stimuli.push_back(&f);
                        }
                        else if (root["type"] == "RandomCircles")
                        {
                            RandomCircles rc = RandomCircles::from_json(root);
                            stimuli.push_back(&rc);
                        }
                        else if (root["type"] == "ColoredWords")
                        {
                            ColoredWords cw = ColoredWords::from_json(root);
                            stimuli.push_back(&cw);
                        }
                    }
                }
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E))
            {
                is_editting = true;
                is_presenting = false;
                current_screen = EDITTING;
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_P))
            {
                RandomCircles *rc = new RandomCircles();
                rc->present();
                is_presenting = true;
                current_screen = PRESENTING;
            }

            break;

        case EDITTING:
            if (true)
            {
                RandomCircles *rc = new RandomCircles();
                int f_current = 0;
                bool show_FPS = true;
                rc->pick();

                while (is_editting)
                {
                    BeginDrawing();
                    ClearBackground(RAYWHITE);
                    SetTargetFPS(rc->FPS);

                    if (show_FPS)
                        DrawFPS(10, 10);

                    if (IsKeyPressed(KEY_F))
                    {
                        show_FPS = !show_FPS;
                    }

                    if (!rc->pick_once)
                        rc->pick();

                    rc->draw();

                    using field = tuple<const char *, int *, int, int>;
                    vector<field> field_vector = {
                        make_tuple("N", &rc->n, 1, 1000),
                        make_tuple("size", &rc->size, 1, 1000),
                        make_tuple("inner", &rc->inner_radius, 1, 1000),
                        make_tuple("outter", &rc->outter_radius, 1, 1000),
                        make_tuple("FPS", &rc->FPS, 10, 1000),
                        make_tuple("duration", &rc->duration, 1, 1000),
                        make_tuple("seed", &rc->random_seed, 0, 1000),
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

                    if (IsKeyPressed(KEY_S))
                    {
                        if (IsKeyDown(KEY_LEFT_CONTROL))
                        {
                            rc->save();
                            // is_editting = false;
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

                    EndDrawing();
                }

                current_screen = MAIN;

                break;
            }
        case PRESENTING:
            while (is_presenting)
            {
            }
            current_screen = REPORT;
            break;
        case REPORT:
            current_screen = MAIN;
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
