#include <iostream>
using namespace std;
 // تضمين المكتبات الأساسية 
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cstdlib>
#include <ctime>

// GLM
#include <glm/glm.hpp>     // للعمليات الحسابية 
#include <glm/gtc/matrix_transform.hpp>   // لإنشاء مصفوفات التحويل
#include <glm/gtc/type_ptr.hpp>   // لتحويل المصفوفات إلى شكل يمكن إرساله إلى الشيدر

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"  //    لتحميل الصور (stb_imageh)تضمين مكتبة 

// ربط المكتبات 
#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")



enum GameState { PLAYING, WIN, GAMEOVER }; // حالات اللعب 
GameState currentState = PLAYING;

// إعدادات النافذة 
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// متغيرات الكاميرا 
/* موقع الكاميرا 
* اتجاه النظر 
* الاتجاه الاعلى بالنسبة للكاميرا 
*/
glm::vec3 cameraPos(0.0f, 1.5f, 8.0f), cameraFront(0.0f, 0.0f, -1.0f), cameraUp(0.0f, 1.0f, 0.0f);
// زاوية الدوران 
float yaw = -90.0f;
float pitch = 0.0f;

float lastX = 640;
float lastY = 360;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;
 
// القفز و الجاذبية 
float jumpVelocity = 0.0f;
bool isGrounded = true;
const float gravity = -15.0f;

//  Structs
struct Bullet { glm::vec3 pos; glm::vec3 dir; float speed = 50.0f; bool active = true; };
struct Zombie { glm::vec3 pos; float speed = 1.5f; bool alive = true; float walkAnim = 0.0f; };
struct Crate { glm::vec3 pos; };

vector<Zombie> zombies;
vector<Crate> staticCrates;
vector<Bullet> bullets;

// vertexShader

const char* vertexShaderSource = R"(
#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec2 aTex;
out vec2 TexCoord;
uniform mat4 model; 
uniform mat4 view;
 uniform mat4 projection;
void main(){
 gl_Position = projection * view * model * vec4(aPos, 1.0);
 TexCoord = aTex; })";

// fragmentShader

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
 uniform vec3 objectColor;
uniform bool useTexture; 
uniform vec2 texScale; 
void main(){ 
    if(useTexture) {
        FragColor = texture(texture1, TexCoord * texScale) * vec4(objectColor, 1.0); 
    }
    else {
        FragColor = vec4(objectColor, 1.0); 
    }
})";

//  OpenGL  يفهمه (Texture) دالة لتحميل الصور و تحويله ل

unsigned int loadTexture(const char* path) {
    unsigned int tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int w, h, ch; stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (data) {
        GLenum f = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    return tex;
}

// مصفوفة المكعب 
float cubeVertices[] = {
    // المواقع (X, Y, Z)      // إحداثيات التكستشر (U, V)

    // الوجه الخلفي (Back Face)
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    // الوجه الأمامي (Front Face)
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    // الوجه الأيسر (Left Face)
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    // الوجه الأيمن (Right Face)
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     // الوجه السفلي (Bottom Face)
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

     // الوجه العلوي (Top Face)
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Zombie Shooter", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    //  Shaders بناء ال 
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    unsigned int shader = glCreateProgram(); 
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);

    // (VBO / VAO)إرسال بيانات المكعب ل كرت الشاشة 
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    //   ( 0 المكان )Position تعريف ال 
    //  ( 1المكان )Texture و ال 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // تحميل الصور 
    unsigned int texWood = loadTexture("wood.jpg"), texWall = loadTexture("wall.jfif"), texland = loadTexture("land.jpg");
     
    //  توزيع المكعبات عشوائياً في المنطقة  
    srand(time(0));
    for (int i = 0; i < 15; i++) staticCrates.push_back({ glm::vec3(rand() % 34 - 17, 0.5f, rand() % 34 - 17) });
    for (int i = 0; i < 5; i++) zombies.push_back({ glm::vec3(rand() % 20 - 10, 0.0f, -12.0f) });

    // (main loop) الحلقة الأساسية 

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window); // معالجة الحركة 
        // الجاذبية و القفز و الوقوف ع الصناديق 
        if (currentState == PLAYING) {
            float currentGroundLevel = 0.0f;
            for (auto& c : staticCrates) {
                float dist = glm::distance(glm::vec2(cameraPos.x, cameraPos.z), glm::vec2(c.pos.x, c.pos.z));
                if (dist < 0.7f && (cameraPos.y - 1.5f) >= (c.pos.y + 0.4f)) {
                    currentGroundLevel = c.pos.y + 0.5f;
                    break;
                }
            }
            jumpVelocity += gravity * deltaTime; cameraPos.y += jumpVelocity * deltaTime;
            if (cameraPos.y <= currentGroundLevel + 1.5f) {
                cameraPos.y = currentGroundLevel + 1.5f;
                jumpVelocity = 0.0f;
                isGrounded = true;
            }
            else {
                isGrounded = false;
            }
        }
        // مسح الشاشة و بدء الرسم 
        glClearColor(0.5f, 0.8f, 0.9f, 1.0f);   // لون السماء 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);
        // (إنشاء مصفوفات الرسم  (الكاميرا 
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        if (currentState == PLAYING) {
            glBindVertexArray(VAO);
            glUniform1i(glGetUniformLocation(shader, "useTexture"), true);

            // 1. الأرضية
            glBindTexture(GL_TEXTURE_2D, texland);
            glUniform2f(glGetUniformLocation(shader, "texScale"), 10.0f, 10.0f);
            glUniform3f(glGetUniformLocation(shader, "objectColor"), 1.0f, 1.0f, 1.0f);
            glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(40, 0.1f, 40));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // 2. الجدران 
            glBindTexture(GL_TEXTURE_2D, texWall);
            glUniform3f(glGetUniformLocation(shader, "objectColor"), 0.98f, 0.45f, 0.15f);

            // نثبت الـ texScale لكل الجدران لأننا سنعتمد على تدوير المجسم
            glUniform2f(glGetUniformLocation(shader, "texScale"), 20.0f, 1.0f);

            for (int i : {-20, 20}) {
                // الجدران الأمامية والخلفية
                model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5f, i));
                model = glm::scale(model, glm::vec3(40, 3, 0.5f));
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // الجدران الجانبية: السر هنا هو التدوير 90 درجة قبل السكيل
                model = glm::translate(glm::mat4(1.0f), glm::vec3(i, 1.5f, 0));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0)); // نفتل الجدار
                model = glm::scale(model, glm::vec3(40, 3, 0.5f)); // نستخدم نفس سكيل الجدار الأمامي
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // 3. الصناديق
            glBindTexture(GL_TEXTURE_2D, texWood);
            glUniform2f(glGetUniformLocation(shader, "texScale"), 1.0f, 1.0f);
            glUniform3f(glGetUniformLocation(shader, "objectColor"), 1.0f, 1.0f, 1.0f);
            for (auto& c : staticCrates) {
                model = glm::translate(glm::mat4(1.0f), c.pos);
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // 4. السلاح 
            glUniform1i(glGetUniformLocation(shader, "useTexture"), false);
            glUniform3f(glGetUniformLocation(shader, "objectColor"), 0.05f, 0.05f, 0.05f);


            glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
            glm::vec3 gunBase = cameraPos + cameraFront * 0.5f - cameraUp * 0.2f + right * 0.25f;
            for (float s : {-0.04f, 0.04f}) {
                glm::mat4 gm = glm::translate(glm::mat4(1.0f), gunBase + right * s);
                gm = glm::rotate(gm, glm::radians(-yaw - 90.0f), glm::vec3(0, 1, 0));
                gm = glm::rotate(gm, glm::radians(pitch), glm::vec3(1, 0, 0));
                gm = glm::scale(gm, glm::vec3(0.05f, 0.05f, 0.6f));
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(gm));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // الطلقات
            for (auto& b : bullets) {
                if (!b.active) continue; b.pos += b.dir * b.speed * deltaTime;
                glUniform3f(glGetUniformLocation(shader, "objectColor"), 1, 0.8f, 0);
                model = glm::translate(glm::mat4(1.0f), b.pos);
                model = glm::scale(model, glm::vec3(0.12f));
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // 5. الزومبي

            bool stillZombies = false;  // اختفاء الزومبي بعد ضربة بالطلقات 
            for (auto& z : zombies) {
                if (!z.alive) continue;
                stillZombies = true;

                z.walkAnim += deltaTime * 5.0f; // تحديث عداد المشي 

                // حسابات ل حركة الزومبي لكي يمشي نحو الاعب 
                glm::vec3 toPlayer = glm::normalize(glm::vec3(cameraPos.x - z.pos.x, 0, cameraPos.z - z.pos.z));
                glm::vec3 nextPos = z.pos + toPlayer * z.speed * deltaTime;

                // (فحص التصادم (لمنع الزومبي من اختراق الجدران و الصناديق 
                bool zHit = false; 
                for (auto& c : staticCrates)
                    if (glm::distance(glm::vec2(nextPos.x, nextPos.z), glm::vec2(c.pos.x, c.pos.z)) < 0.8f) {
                        zHit = true;
                        break;
                    }
                // اذا في عائق ل حركة الزومبي يكون قادر ع تخطيها 
                if (zHit) {
                    glm::vec3 avoidDir = glm::normalize(glm::cross(toPlayer, glm::vec3(0, 1, 0))); z.pos += avoidDir * z.speed * deltaTime;
                }
                else z.pos = nextPos;

                glUniform3f(glGetUniformLocation(shader, "objectColor"), 0.1f, 0.8f, 0.1f);// رسم الرأس و تلونه بالأخضر 
                model = glm::translate(glm::mat4(1.0f), z.pos + glm::vec3(0, 1.45f, 0));
                model = glm::scale(model, glm::vec3(0.4f));
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);


                glUniform3f(glGetUniformLocation(shader, "objectColor"), 1, 0, 0); // رسم العيون و تلوينها بالأحمر 
                model = glm::translate(glm::mat4(1.0f), z.pos + glm::vec3(0, 1.5f, 0.21f));
                model = glm::scale(model, glm::vec3(0.1f));
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // رسم جسم الزومبي 
                glUniform3f(glGetUniformLocation(shader, "objectColor"), 0.05f, 0.05f, 0.05f);
                model = glm::translate(glm::mat4(1.0f), z.pos + glm::vec3(0, 0.8f, 0));
                model = glm::scale(model, glm::vec3(0.5f, 0.9f, 0.3f));
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // اليدين
                glUniform3f(glGetUniformLocation(shader, "objectColor"), 0.1f, 0.8f, 0.1f);
                for (float s : {-0.3f, 0.3f}) { // اليمنى و اليسرى 
                    model = glm::translate(glm::mat4(1.0f), z.pos + glm::vec3(s, 1.0f, 0.3f));
                    model = glm::scale(model, glm::vec3(0.12f, 0.12f, 0.5f)); // يد ممدودة للأمام 
                    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                // الساقين مع حركة المشي 
                float lM = sin(z.walkAnim) * 0.25f;
                for (float s : {-0.15f, 0.15f}) {
                    model = glm::translate(glm::mat4(1.0f), z.pos + glm::vec3(s, 0.25f, (s > 0 ? lM : -lM)));
                    model = glm::scale(model, glm::vec3(0.2f, 0.5f, 0.2f));
                    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                //فحص إصابة الزمبي بالرصاص 
                for (auto& b : bullets) 
                    if (b.active && glm::distance(b.pos, z.pos + glm::vec3(0, 1, 0)) < 0.8f) {
                        z.alive = false; // اختفاء الزومبي 
                        b.active = false;// اختفاء الرصاصة 
                    }
                // (فحص لمس الزومبي للاعب (خسارة  
                float distH = glm::distance(glm::vec2(cameraPos.x, cameraPos.z), glm::vec2(z.pos.x, z.pos.z));
                if (distH < 0.7f && abs(cameraPos.y - (z.pos.y + 1.0f)) < 1.0f) {
                    currentState = GAMEOVER; glfwSetWindowShouldClose(window, true);
                }
            }
            // حالة الفوز اذا تم قتل كل الزومبي 
            if (!stillZombies) {
                currentState = WIN;
                glfwSetWindowShouldClose(window, true);
            }
        }
        //   و معالجة الاحداث Buffers تبديل ال 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // تنظيف الذاكرة عند اغلاق اللعبة 
    glfwTerminate();
    system("cls"); // مسح شاشة الكونسول 

    if (currentState == WIN) 
        cout << " --- WIN! --- " << endl;
    else
        cout << " --- GAME OVER! --- " << endl;
    return 0;
}
 // دالة معالجة مدخلات لوحة المفاتيح 
void processInput(GLFWwindow* window) {
    if (currentState != PLAYING) return;
    float s = 6.0f * deltaTime;
    glm::vec3 nextPos = cameraPos;
    glm::vec3 fs = glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) nextPos += s * fs;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) nextPos -= s * fs;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) nextPos -= glm::normalize(glm::cross(fs, cameraUp)) * s;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) nextPos += glm::normalize(glm::cross(fs, cameraUp)) * s;

    // فحص التصادم لمنع اللاعب من اختراق الصناديق و الجدران 
    bool collision = false;
    if (nextPos.x > 19.3f || nextPos.x < -19.3f || nextPos.z > 19.3f || nextPos.z < -19.3f) collision = true;
    for (auto& c : staticCrates) {
        float d = glm::distance(glm::vec2(nextPos.x, nextPos.z), glm::vec2(c.pos.x, c.pos.z));
        if (d < 1.0f && (cameraPos.y - 1.5f) < (c.pos.y + 0.4f)) {
            collision = true;
            break;
        }
    }

    if (!collision) { // تحديث موقع الاعب اذا لم يحدث تصادم 
        cameraPos.x = nextPos.x;
        cameraPos.z = nextPos.z;
    }
    // القفز رز space
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        jumpVelocity = 6.0f;
        isGrounded = false;
    }
}

//( دالة التحكم بالرؤية (حركة الماوس 
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float x = (xpos - lastX) * 0.12f;
    float y = (lastY - ypos) * 0.12f;
    lastX = xpos;
    lastY = ypos;
    yaw += x;
    pitch += y;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    cameraFront = glm::normalize(glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw)) * cos(glm::radians(pitch))));
}
 // دالة اطلاق الرصاص 
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) bullets.push_back({ cameraPos + cameraFront * 0.5f, cameraFront });
}