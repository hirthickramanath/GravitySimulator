#include <stdio.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "gravity.h"

void animate(const char *filename) {
    FILE *fp;
    static FRAME f[FRAMES];
    BODY b[2];
    char line[256], name[20], symbol;
    int rows = 0;
    int lastStep = 0;
    int frame = 0;
    int step;
    double t, mass, x, y, r, v, force, TE;
    double x0 = 0;
    double y0 = 0;
    double farthest = 1.0;
    double scale;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("\n Error opening file!\n");
        return;
    }

    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp) != NULL)
        rows++;
    lastStep = rows / 2 - 1;
    if (lastStep < 0) {
        printf("\n %s has insufficient data for animation.\n", filename);
        fclose(fp);
        return;
    }

    rewind(fp);
    fgets(line, sizeof(line), fp);

    for (int row=0; row < rows; row++) {
        if (fscanf(fp, " %d,%lf,%19[^,],%c,%le,%le,%le,%le,%le,%le,%le\n", &step, &t, name, &symbol, &mass, &x, &y, &r, &v, &force, &TE) != 11) {
            printf("\n Error reading data from %s.\n", filename);
            break;
        }

        int i = row % 2;
        if (step == 0) {
            strcpy(b[i].name, name);
            b[i].symbol = symbol;
            b[i].mass = mass;
        }

        if (i == 0) {
            x0 = x;
            y0 = y;
            continue;
        }

        while (frame < FRAMES && (long long)frame * lastStep / (FRAMES - 1) == step) {
            f[frame].t = t;
            f[frame].x[0] = x0;
            f[frame].y[0] = y0;
            f[frame].x[1] = x;
            f[frame].y[1] = y;
            f[frame].r = r;
            f[frame].f = force;
            f[frame].v = v;
            f[frame].TE = TE;
            frame++;
        }
    }

    fclose(fp);

    for (int k=0; k<FRAMES; k++) {
        for (int i=0; i<2; i++) {
            if (fabs(f[k].x[i]) > farthest)
                farthest = fabs(f[k].x[i]);
            if (fabs(f[k].y[i]) > farthest)
                farthest = fabs(f[k].y[i]);
        }
    }

    scale = 9.0 / farthest;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Gravity Simulation");
    SetTargetFPS(FPS);

    Model model[2];
    Texture tex[2];
    Color col[2];
    float radius[2];

    for (int i=0; i<2; i++) {
        switch (b[i].symbol) {
            case 'S':
                col[i] = GOLD;
                break;
            case 'E':
                col[i] = SKYBLUE;
                break;
            case 'M':
                col[i] = LIGHTGRAY;
                break;
            case 'J':
                col[i] = BEIGE;
                break;
            case 'C':
                col[i] = (Color){ 120, 220, 225, 255 };
                break;
            case 'A':
                col[i] = ORANGE;
                break;
            case 'B':
                col[i] = (Color){ 255, 120, 90, 255 };
                break;
            default:
                col[i] = (i == 0) ? GOLD : SKYBLUE;
                break;
        }

        radius[i] = 0.3f + 0.08f * (float)log10(b[i].mass / 1e14);
        if (radius[i] < 0.3f)
            radius[i] = 0.3f;

        Color dark = { (unsigned char)(col[i].r * 0.55f), (unsigned char)(col[i].g * 0.55f), (unsigned char)(col[i].b * 0.55f), 255};
        Image img = GenImageChecked(256, 128, 8, 4, col[i], dark);
        tex[i] = LoadTextureFromImage(img);
        UnloadImage(img);

        model[i] = LoadModelFromMesh(GenMeshSphere(1.0f, 32, 32));
        model[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex[i];
    }

    int starX[STARS], starY[STARS];
    for (int i=0; i<STARS; i++) {
        starX[i] = GetRandomValue(0, SCREEN_W);
        starY[i] = GetRandomValue(0, SCREEN_H);
    }

    Camera3D camera = {0};
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    int k = 0;
    float spin = 0.0f;
    float camAngle = 0.6f;

    while (!WindowShouldClose()) {
        spin += 1.5f;
        camAngle += 0.0015f;
        camera.position = (Vector3){ 22.0f * sinf(camAngle), 13.0f, 22.0f * cosf(camAngle) };

        BeginDrawing();
        ClearBackground((Color){ 8, 10, 20, 255 });

        for (int i=0; i<STARS; i++)
            DrawPixel(starX[i], starY[i], (i % 3 == 0) ? WHITE : GRAY);

        BeginMode3D(camera);
            for (int i=0; i<2; i++) {
                for (int j=1; j<=k; j++) {
                    Vector3 a = { (float)(f[j - 1].x[i] * scale), 0.0f, (float)(-f[j - 1].y[i] * scale) };
                    Vector3 c = { (float)(f[j].x[i] * scale),     0.0f, (float)(-f[j].y[i] * scale) };
                    DrawLine3D(a, c, Fade(col[i], 0.6f));
                }

                Vector3 pos  = { (float)(f[k].x[i] * scale), 0.0f, (float)(-f[k].y[i] * scale) };
                Vector3 size = { radius[i], radius[i], radius[i] };
                DrawModelEx(model[i], pos, (Vector3){ 0.0f, 1.0f, 0.0f }, spin * (i == 0 ? 0.6f : 1.0f), size, WHITE);
            }
        EndMode3D();

        DrawText("GRAVITY SIMULATION", 20, 18, 24, RAYWHITE);
        DrawText(b[0].name, 20, 54, 20, col[0]);
        DrawText(b[1].name, 20 + MeasureText(b[0].name, 20) + 30, 54, 20, col[1]);
        DrawText(TextFormat("Time      : %10.1f days", f[k].t / 86400.0), 20, 92, 20, LIGHTGRAY);
        DrawText(TextFormat("Distance  : %10.3e m", f[k].r), 20, 118, 20, LIGHTGRAY);
        DrawText(TextFormat("Force     : %10.3e N", f[k].f), 20, 144, 20, LIGHTGRAY);
        DrawText(TextFormat("Speed     : %10.0f m/s", f[k].v), 20, 170, 20, LIGHTGRAY);
        DrawText(TextFormat("Energy    : %10.3e J", f[k].TE), 20, 196, 20, LIGHTGRAY);

        DrawRectangle(20, SCREEN_H - 40, SCREEN_W - 40, 8, (Color){ 40, 44, 60, 255 });
        DrawRectangle(20, SCREEN_H - 40, (SCREEN_W - 40) * (k + 1) / FRAMES, 8, SKYBLUE);

        if (k == FRAMES - 1)
            DrawText("Simulation finished - press ESC or close the window", 20, SCREEN_H - 70, 20, RAYWHITE);
        else
            DrawText("ESC to stop", SCREEN_W - 140, 20, 20, GRAY);

        EndDrawing();

        if (k < FRAMES - 1)
            k++;
    }

    for (int i=0; i<2; i++) {
        UnloadModel(model[i]);
        UnloadTexture(tex[i]);
    }

    CloseWindow();
}