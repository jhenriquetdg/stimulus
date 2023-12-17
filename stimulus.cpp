#define RAYLIB_IMPLEMENTATION
#include <./raylib.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <jsoncpp/json/json.h>

#include <stdio.h>
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

Font font = GetFontDefault();

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
    RANDOM_CIRCLES,
    COLORED_WORDS,
} Stim;

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
    virtual std::string to_string() = 0;

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

class Fixing : public Stimulus
{
public:
    const char *sign = "+";
    int font_size = 70;
    int center_x = middle_x_screen;
    int center_y = middle_y_screen;
    Color color = LIGHTGRAY;

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

    std::string to_string() override
    {
        std::string result = "Fixing(" +
                             std::to_string(this->font_size) + "," +
                             std::to_string(this->center_x) + "," +
                             std::to_string(this->center_y) + "," +
                             std::to_string(this->FPS) + "," +
                             std::to_string(this->duration) + "," +
                             std::to_string(this->repetitions) + "," +
                             std::to_string(this->random_seed) +
                             ")";
        return result;
    }

    static Fixing from_json(Json::Value root)
    {
        Fixing *s = new Fixing;

        if (root["type"] == "Fixing")
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

    string to_string() override
    {
        std::string result = "RandomCircles(" +
                             std::to_string(this->n) + "," +
                             std::to_string(this->size) + "," +
                             std::to_string(this->inner_radius) + "," +
                             std::to_string(this->outter_radius) + "," +
                             std::to_string(this->FPS) + "," +
                             std::to_string(this->duration) + "," +
                             std::to_string(this->repetitions) + "," +
                             std::to_string(this->random_seed) +
                             ")";

        return result;
    }

    static RandomCircles from_json(Json::Value root)
    {
        int f = 0;

        RandomCircles *rc = new RandomCircles();

        if (root["type"] == "RandomCircles")
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

using word_color = pair<char const *, Color>;
class ColoredWords : public Stimulus
{
public:
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

    std::string to_string() override
    {
        std::string result = "ColoredWords(" +
                             std::to_string(this->font_size) + "," +
                             std::to_string(this->FPS) + "," +
                             std::to_string(this->duration) + "," +
                             std::to_string(this->repetitions) + "," +
                             std::to_string(this->random_seed) +
                             ")";
        return result;
    }

    static ColoredWords from_json(Json::Value root)
    {
        ColoredWords *s = new ColoredWords;

        if (root["type"] == "ColoredWords")
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

void load_from_disk(vector<Stimulus *> *stimuli)
{
    stimuli->clear();
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
                cout << "Fixing " << endl;
                Fixing *f = new Fixing;
                cout << "Fixing1 " << endl;
                *f = Fixing::from_json(root);
                cout << "Fixing2 " << endl;
                stimuli->push_back(f);
                cout << f->to_string() << endl;
            }
            else if (root["type"] == "RandomCircles")
            {
                RandomCircles *rc = new RandomCircles;
                *rc = RandomCircles::from_json(root);
                stimuli->push_back(rc);
                cout << rc->to_string() << endl;
            }
            else if (root["type"] == "ColoredWords")
            {
                ColoredWords *cw = new ColoredWords;
                *cw = ColoredWords::from_json(root);
                stimuli->push_back(cw);
                cout << cw->to_string() << endl;
            }
        }
    }
}

#define COLOR_ACCENT ColorFromHSV(225, 0.75, 0.8)
#define COLOR_BACKGROUND DARKGRAY
#define COLOR_TRACK_PANEL_BACKGROUND ColorBrightness(COLOR_BACKGROUND, -0.1)
#define COLOR_TRACK_BUTTON_BACKGROUND ColorBrightness(COLOR_BACKGROUND, 0.15)
#define COLOR_TRACK_BUTTON_HOVEROVER ColorBrightness(COLOR_TRACK_BUTTON_BACKGROUND, 0.15)
#define COLOR_TRACK_BUTTON_SELECTED COLOR_ACCENT

typedef enum
{
    BS_NONE = 0,      // 00
    BS_HOVEROVER = 1, // 01
    BS_CLICKED = 2,   // 10
} Button_State;

static void stimuli_panel(vector<Stimulus *> stimuli, Stimulus *selected, int *selected_index, Rectangle panel_boundary)
{

    auto button_with_id = [selected_index](uint64_t id, Rectangle boundary)
    {
        Vector2 mouse = GetMousePosition();
        int hoverover = CheckCollisionPointRec(mouse, boundary);

        if (hoverover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            cout << "Mouse clicked!" << endl;
            *selected_index = id;
            cout << *selected_index << endl;
            hoverover = BS_CLICKED;
        }

        return hoverover;
    };

    DrawRectangleRec(panel_boundary, COLOR_TRACK_PANEL_BACKGROUND);

    Vector2 mouse = GetMousePosition();

    float scroll_bar_width = panel_boundary.width * 0.03;

    float item_size = panel_boundary.width * 0.2;
    float visible_area_size = panel_boundary.height;
    float entire_scrollable_area = item_size * stimuli.size();

    static float panel_scroll = 0;
    static float panel_velocity = 0;
    panel_velocity *= 0.9;
    if (CheckCollisionPointRec(mouse, panel_boundary))
    {
        panel_velocity += GetMouseWheelMove() * item_size * 8;
    }
    panel_scroll -= panel_velocity * GetFrameTime();

    static bool scrolling = false;
    static float scrolling_mouse_offset = 0.0f;
    if (scrolling)
    {
        panel_scroll = (mouse.y - panel_boundary.y - scrolling_mouse_offset) / visible_area_size * entire_scrollable_area;
    }

    float min_scroll = 0;
    if (panel_scroll < min_scroll)
        panel_scroll = min_scroll;
    float max_scroll = entire_scrollable_area - visible_area_size;
    if (max_scroll < 0)
        max_scroll = 0;
    if (panel_scroll > max_scroll)
        panel_scroll = max_scroll;
    float panel_padding = item_size * 0.1;

    BeginScissorMode(panel_boundary.x, panel_boundary.y, panel_boundary.width, panel_boundary.height);
    for (size_t i = 0; i < stimuli.size(); i++)
    {
        Rectangle item_boundary = {
            .x = panel_boundary.x + panel_padding,
            .y = i * item_size + panel_boundary.y + panel_padding - panel_scroll,
            .width = panel_boundary.width - panel_padding * 2 - scroll_bar_width,
            .height = item_size - panel_padding * 2,
        };
        Color color;
        if ((i != *selected_index))
        {
            int state = button_with_id(i, GetCollisionRec(panel_boundary, item_boundary));
            if (state & BS_HOVEROVER)
            {
                color = COLOR_TRACK_BUTTON_HOVEROVER;
            }
            else
            {
                color = COLOR_TRACK_BUTTON_BACKGROUND;
            }
            if (state & BS_CLICKED)
            {
                selected = stimuli[i];
                *selected_index = i;
            }
        }
        else
        {
            color = COLOR_TRACK_BUTTON_SELECTED;
        }

        DrawRectangleRounded(item_boundary, 0.2, 20, color);

        string aux = stimuli[i]->to_string();
        const char *text = aux.c_str();

        float fontSize = item_boundary.height * 0.5;
        float text_padding = item_boundary.width * 0.05;
        Vector2 size = MeasureTextEx(font, text, fontSize, 0);
        Vector2 position = {
            .x = item_boundary.x + text_padding,
            .y = item_boundary.y + item_boundary.height * 0.333 - size.y * 0.5,
        };

        DrawTextEx(font, text, position, fontSize, 0, WHITE);
    }

    if (entire_scrollable_area > visible_area_size)
    {
        float t = visible_area_size / entire_scrollable_area;
        float q = panel_scroll / entire_scrollable_area;
        Rectangle scroll_bar_area = {
            .x = panel_boundary.x + panel_boundary.width - scroll_bar_width,
            .y = panel_boundary.y,
            .width = scroll_bar_width,
            .height = panel_boundary.height,
        };

        Rectangle scroll_bar_boundary = {
            .x = panel_boundary.x + panel_boundary.width - scroll_bar_width,
            .y = panel_boundary.y + panel_boundary.height * q,
            .width = scroll_bar_width,
            .height = panel_boundary.height * t,
        };
        DrawRectangleRounded(scroll_bar_boundary, 0.8, 20, COLOR_TRACK_BUTTON_BACKGROUND);

        if (scrolling)
        {
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                scrolling = false;
            }
        }
        else
        {
            if (CheckCollisionPointRec(mouse, scroll_bar_boundary))
            {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    scrolling = true;
                    scrolling_mouse_offset = mouse.y - scroll_bar_boundary.y;
                }
            }
            else if (CheckCollisionPointRec(mouse, scroll_bar_area))
            {
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                {
                    if (mouse.y < scroll_bar_boundary.y)
                    {
                        panel_velocity += item_size * 16;
                    }
                    else if (scroll_bar_boundary.y + scroll_bar_boundary.height < mouse.y)
                    {
                        panel_velocity += -item_size * 16;
                    }
                }
            }
        }
    }

    EndScissorMode();
}

int main(void)
{
    // Setting raylib variables
    InitWindow(screen_width, screen_height, "Stimulus");
    SetTargetFPS(screen_FPS);

    filesystem::create_directories("./files");
    filesystem::create_directories("./files/stimuli");
    filesystem::create_directories("./files/experiments");
    filesystem::create_directories("./files/people");

    vector<Stimulus *> stimuli = {};
    vector<Stimulus *> exp_stimuli = {};

    load_from_disk(&stimuli);

    Screen current_screen = LOGO;

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

    Stim current_stimulus_type = RANDOM_CIRCLES;

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
        {
            DrawText("Stimuli", 5, screen_height - 50, 50, LIGHTGRAY);
            skip_count++;
            if (IsKeyPressed(KEY_ENTER) || skip_count > logo_time * screen_FPS)
            {
                current_screen = MAIN;
                skip_count = 0;
            }
            break;
        }
        case MAIN:
        {
            DrawText("Main", 5, screen_height - 50, 50, LIGHTGRAY);
            skip_count++;

            Stimulus *left_stimulus = 0;
            int left_stimulus_index = 0;
            stimuli_panel(stimuli,
                          left_stimulus,
                          &left_stimulus_index,
                          (Rectangle){
                              .x = 0,
                              .y = 30,
                              .width = 300,
                              .height = 400,
                          });

            Stimulus *right_stimulus = 0;
            int right_stimulus_index = 0;
            stimuli_panel(exp_stimuli,
                          right_stimulus,
                          &right_stimulus_index,
                          (Rectangle){
                              .x = 400,
                              .y = 30,
                              .width = 300,
                              .height = 400,
                          });

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_L))
            {
                load_from_disk(&stimuli);
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
        }
        case EDITTING:
        {
            using field = tuple<const char *, int *, int, int>;

            vector<field> fixing_fields = {};
            Fixing *fixing_stim = new Fixing();

            fixing_fields.push_back(make_tuple("font_size", &fixing_stim->font_size, 1, 1000));
            fixing_fields.push_back(make_tuple("center_x", &fixing_stim->center_x, 1, 1000));
            fixing_fields.push_back(make_tuple("center_y", &fixing_stim->center_y, 1, 1000));
            fixing_fields.push_back(make_tuple("FPS", &fixing_stim->FPS, 10, 1000));
            fixing_fields.push_back(make_tuple("duration", &fixing_stim->duration, 1, 1000));
            fixing_fields.push_back(make_tuple("seed", &fixing_stim->random_seed, 0, 1000));

            vector<field> rc_fields = {};
            RandomCircles *rc_stim = new RandomCircles();

            rc_fields.push_back(make_tuple("N", &rc_stim->n, 1, 1000));
            rc_fields.push_back(make_tuple("size", &rc_stim->size, 1, 1000));
            rc_fields.push_back(make_tuple("inner", &rc_stim->inner_radius, 1, 1000));
            rc_fields.push_back(make_tuple("outter", &rc_stim->outter_radius, 1, 1000));
            rc_fields.push_back(make_tuple("FPS", &rc_stim->FPS, 10, 1000));
            rc_fields.push_back(make_tuple("duration", &rc_stim->duration, 1, 1000));
            rc_fields.push_back(make_tuple("seed", &rc_stim->random_seed, 0, 1000));

            vector<field> cw_fields = {};
            ColoredWords *cw_stim = new ColoredWords();

            cw_fields.push_back(make_tuple("font_size", &cw_stim->font_size, 1, 100));
            cw_fields.push_back(make_tuple("FPS", &cw_stim->FPS, 10, 1000));
            cw_fields.push_back(make_tuple("duration", &cw_stim->duration, 1, 1000));
            cw_fields.push_back(make_tuple("seed", &cw_stim->random_seed, 0, 1000));

            int f_current = 0;
            bool show_FPS = true;

            Stim editting_type = RANDOM_CIRCLES;

            fixing_stim->pick();

            while (is_editting)
            {
                BeginDrawing();
                ClearBackground(RAYWHITE);

                if (show_FPS)
                    DrawFPS(10, 10);

                if (IsKeyPressed(KEY_F))
                {
                    show_FPS = !show_FPS;
                }

                SetTargetFPS(fixing_stim->FPS);

                Stimulus *editting_stimulus = 0;

                vector<field> field_vector;
                Stim next_editting_type;
                switch (editting_type)
                {
                case FIXING:
                    editting_stimulus = fixing_stim;
                    field_vector = fixing_fields;
                    next_editting_type = COLORED_WORDS;
                    editting_stimulus->pick_once = true;
                    break;
                case COLORED_WORDS:
                    editting_stimulus = cw_stim;
                    field_vector = cw_fields;
                    next_editting_type = RANDOM_CIRCLES;
                    editting_stimulus->pick_once = true;
                    break;
                default:
                    editting_stimulus = rc_stim;
                    field_vector = rc_fields;
                    next_editting_type = FIXING;
                    editting_stimulus->pick_once = false;
                    break;
                }

                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ENTER))
                    editting_type = next_editting_type;

                if (!editting_stimulus->pick_once || IsKeyPressed(KEY_SPACE))
                    editting_stimulus->pick();
                editting_stimulus->draw();

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

                cout << field_index << endl;

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
                        editting_stimulus->save();
                    }
                }

                if (IsKeyPressed(KEY_C))
                {
                    if (IsKeyDown(KEY_LEFT_CONTROL))
                    {

                        delete cw_stim;
                        delete rc_stim;
                        delete fixing_stim;
                        // delete editting_stimulus;
                        is_editting = false;
                    }
                }

                EndDrawing();
            }

            current_screen = MAIN;

            break;
        }
        case PRESENTING:
        {
            while (is_presenting)
            {
            }
            current_screen = REPORT;
            break;
        }
        case REPORT:
        {
            current_screen = MAIN;
            break;
        }
        default:
            break;
        }

        EndDrawing();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
