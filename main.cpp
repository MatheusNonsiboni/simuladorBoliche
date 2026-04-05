// g++ main.cpp -o main.exe -lopengl32 -lglu32 -lfreeglut -mconsole
// ./main.exe
#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>


// variaveis globais

// camera
float camX = 0.0f;
float camY = 3.0f;
float camZ = 15.0f;
float lookX = 0.0f;
float lookY = 0.0f;
float lookZ = 0.0f;

// Interaçâo
float posicaoBolaX = 0.0f;   // Posição lateral da bola
float velocidadeBola = 0.0f; // Velocidade de lançamento
float posicaoBolaZ = 10.0f;  // Posição Z da bola
float rotacaoBola = 0.0f;    // Rotação da bola enquanto rola

// braço limpador (sweeper)
float anguloLimpador = 90.0f; // Rotação do braço (90 = erguido/repouso, 0 = abaixado)
bool limpando = false;       // Se está animando
int faseLimpador = 0;        // Fases da animação (0 = idle, 1 = descendo, 2 = limpando, 3 = subindo)
float tempoLimpador = 0.0f;  // Timer auxiliar para fases do sweeper

// Pinos
const int NUM_PINOS = 10;
struct Pino {
    float x, y, z;
    bool derrubado;
    float anguloAlvo;  // angulo final (e.g., 90.0f)
    float anguloAtual; // angulo atual (para animação de queda)
} pinos[NUM_PINOS];

bool jogoLancado = false; // Se a bola foi lançada

// Textura da pista
GLuint texturaPista = 0;
const int TEX_PISTA_LARGURA = 512;
const int TEX_PISTA_ALTURA = 64;

// Declaração de funções
void inicializarPinos();
void init();
void reshape(int w, int h);
void display();
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void timer(int value);
void desenharPino(const Pino& pino);
void desenharBola(float x, float y, float z);
void desenharPista();
void atualizarLogicaJogo(float deltaTime);
void detectarColisoes();
void desenharLimpador();
void reiniciarJogo();
void criarTexturaPista();

// funções de desenho

// Desenha um pino
void desenharPino(const Pino& pino) {
    glPushMatrix();
    glTranslatef(pino.x, pino.y, pino.z);

    // Se derrubado, rotaciona em torno da base para simular queda
    if (pino.derrubado) {
        glRotatef(pino.anguloAtual, 1.0f, 0.0f, 0.0f);
    }

    // Rotaciona -90 graus no eixo X para o cilindro ficar de pe (eixo Z -> eixo Y)
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    // Material do pino (branco, especular moderado)
    GLfloat pino_ambient[] = {0.12f, 0.12f, 0.12f, 1.0f};
    GLfloat pino_diffuse[] = {0.95f, 0.95f, 0.95f, 1.0f};
    GLfloat pino_specular[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat pino_shininess = 30.0f;

    glMaterialfv(GL_FRONT, GL_AMBIENT, pino_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, pino_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, pino_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, pino_shininess);

    // Base (pequeno cilindro)
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluCylinder(quad, 0.15, 0.1, 0.1, 20, 20);

    // Corpo principal
    glTranslatef(0.0f, 0.0f, 0.1f);
    gluCylinder(quad, 0.1, 0.15, 0.4, 20, 20);

    // Topo arredondado
    glTranslatef(0.0f, 0.0f, 0.4f);
    glutSolidSphere(0.15, 20, 20);

    gluDeleteQuadric(quad);
    glPopMatrix();
}

// Desenha a bola (vermelha)
void desenharBola(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);

    // Rotaçãoo para simular rolamento
    glRotatef(rotacaoBola, 1.0f, 0.0f, 0.0f);

    // Material da bola (vermelho brilhante moderado)
    GLfloat bola_ambient[] = {0.05f, 0.0f, 0.0f, 1.0f};
    GLfloat bola_diffuse[] = {0.8f, 0.05f, 0.05f, 1.0f};
    GLfloat bola_specular[] = {0.9f, 0.9f, 0.9f, 1.0f};
    GLfloat bola_shininess = 60.0f;

    glMaterialfv(GL_FRONT, GL_AMBIENT, bola_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, bola_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, bola_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, bola_shininess);

    glutSolidSphere(0.3, 30, 30);

    glPopMatrix();
}

// Desenha a pista texturizada
void desenharPista() {
    // Material base da pista (madeira clara)
    GLfloat pista_ambient[] = {0.08f, 0.04f, 0.02f, 1.0f};
    GLfloat pista_diffuse[] = {0.5f, 0.25f, 0.08f, 1.0f};
    GLfloat pista_specular[] = {0.05f, 0.05f, 0.05f, 1.0f};
    GLfloat pista_shininess = 10.0f;

    glMaterialfv(GL_FRONT, GL_AMBIENT, pista_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, pista_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, pista_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, pista_shininess);

    // Definição do plano da pista
    float y = 0.0f;
    float x1 = -1.0f, x2 = 1.0f;
    float z1 = -10.0f, z2 = 10.0f;

    // Ativa textura
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaPista);

    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);

        // Mapeamento UV: repetir a textura ao longo do comprimento
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y, z1);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y, z1);
        glTexCoord2f(1.0f, 8.0f); glVertex3f(x2, y, z2);
        glTexCoord2f(0.0f, 8.0f); glVertex3f(x1, y, z2);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Bordas laterais (madeira escura)
    glPushMatrix();
        GLfloat borda_ambient[] = {0.06f, 0.03f, 0.01f, 1.0f};
        GLfloat borda_diffuse[] = {0.25f, 0.12f, 0.04f, 1.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT, borda_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, borda_diffuse);

        glBegin(GL_QUADS);
            // esquerda
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(-1.2f, y-0.01f, z1);
            glVertex3f(-1.0f, y-0.01f, z1);
            glVertex3f(-1.0f, y-0.01f, z2);
            glVertex3f(-1.2f, y-0.01f, z2);

            // direita
            glVertex3f(1.0f, y-0.01f, z1);
            glVertex3f(1.2f, y-0.01f, z1);
            glVertex3f(1.2f, y-0.01f, z2);
            glVertex3f(1.0f, y-0.01f, z2);
        glEnd();
    glPopMatrix();
}

// Inicializa as posiçôes dos 10 pinos
void inicializarPinos() {
    float espacamento = 0.5f;
    float startZ = -9.0f; // fundo da pista (fileira de 4 fica aqui)
    float r = espacamento * 0.866f;
    int count = 0;

    // Linha 4 (fundo): 4 pinos - mais longe do jogador
    pinos[count++] = {-1.5f * espacamento, 0.3f, startZ, false, 90.0f, 0.0f};
    pinos[count++] = {-0.5f * espacamento, 0.3f, startZ, false, 90.0f, 0.0f};
    pinos[count++] = {0.5f * espacamento, 0.3f, startZ, false, 90.0f, 0.0f};
    pinos[count++] = {1.5f * espacamento, 0.3f, startZ, false, 90.0f, 0.0f};

    // Linha 3: 3 pinos
    pinos[count++] = {-espacamento, 0.3f, startZ + r, false, 90.0f, 0.0f};
    pinos[count++] = {0.0f, 0.3f, startZ + r, false, 90.0f, 0.0f};
    pinos[count++] = {espacamento, 0.3f, startZ + r, false, 90.0f, 0.0f};

    // Linha 2: 2 pinos
    pinos[count++] = {-espacamento / 2.0f, 0.3f, startZ + 2 * r, false, 90.0f, 0.0f};
    pinos[count++] = {espacamento / 2.0f, 0.3f, startZ + 2 * r, false, 90.0f, 0.0f};

    // Linha 1 (frente): 1 pino - mais perto do jogador
    pinos[count++] = {0.0f, 0.3f, startZ + 3 * r, false, 90.0f, 0.0f};

    // Ajuste e zeragem de parametros
    for (int i = 0; i < NUM_PINOS; ++i) {
        pinos[i].y = 0.3f;
        pinos[i].derrubado = false;
        pinos[i].anguloAtual = 0.0f;
        pinos[i].anguloAlvo = 90.0f;
    }
}

// Cria textura procedural para a pista de boliche
void criarTexturaPista() {
    std::vector<unsigned char> data(TEX_PISTA_LARGURA * TEX_PISTA_ALTURA * 3);

    for (int y = 0; y < TEX_PISTA_ALTURA; ++y) {
        for (int x = 0; x < TEX_PISTA_LARGURA; ++x) {
            int idx = (y * TEX_PISTA_LARGURA + x) * 3;

            float fx = (float)x / TEX_PISTA_LARGURA;
            float stripe = sinf(fx * 60.0f) * 0.5f + 0.5f;
            float noise = (float)(((x * 17) ^ (y * 131)) & 255) / 255.0f;
            float t = 0.35f + 0.5f * stripe + 0.05f * noise;

            unsigned char r = (unsigned char)fmin(255, (int)(180.0f * t + 20.0f));
            unsigned char g = (unsigned char)fmin(255, (int)(100.0f * t + 10.0f));
            unsigned char b = (unsigned char)fmin(255, (int)(50.0f * t + 5.0f));

            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
        }
    }

    glGenTextures(1, &texturaPista);
    glBindTexture(GL_TEXTURE_2D, texturaPista);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, TEX_PISTA_LARGURA, TEX_PISTA_ALTURA, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());

    glBindTexture(GL_TEXTURE_2D, 0);
}

// Inicializa OpenGL e cena
void init() {
    // Fundo azul escuro
    glClearColor(0.05f, 0.05f, 0.15f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    // Iluminação
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    GLfloat global_ambient[] = {0.12f, 0.12f, 0.12f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Luz principal (posicional)
    GLfloat luz0_pos[] = {0.0f, 6.0f, 2.0f, 1.0f};
    GLfloat luz0_amb[] = {0.05f, 0.05f, 0.05f, 1.0f};
    GLfloat luz0_diff[] = {0.95f, 0.95f, 0.95f, 1.0f};
    GLfloat luz0_spec[] = {0.6f, 0.6f, 0.6f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, luz0_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, luz0_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz0_diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz0_spec);

    // Luz direcional lateral (reduz brilho)
    GLfloat luz1_dir[] = { -0.3f, -1.0f, -0.5f, 0.0f }; // w=0 -> direcional
    GLfloat luz1_amb[] = {0.02f, 0.02f, 0.02f, 1.0f};
    GLfloat luz1_diff[] = {0.35f, 0.35f, 0.35f, 1.0f};
    GLfloat luz1_spec[] = {0.2f, 0.2f, 0.2f, 1.0f};

    glLightfv(GL_LIGHT1, GL_POSITION, luz1_dir);
    glLightfv(GL_LIGHT1, GL_AMBIENT, luz1_amb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, luz1_diff);
    glLightfv(GL_LIGHT1, GL_SPECULAR, luz1_spec);

    // Criar textura procedural e inicializar pinos
    criarTexturaPista();
    inicializarPinos();
}

// Função de redimensionamento (Viewport/Projeção)
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float ratio = (float)w / (float)h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
}

// Desenha o limpador (sweeper)
void desenharLimpador() {
    glPushMatrix();
        // Move para o fundo da pista
        glTranslatef(0.0f, 4.0f, -10.0f);

        // rotação do pai (angulo do limpador)
        glRotatef(anguloLimpador, 1.0f, 0.0f, 0.0f);

        // braço preto fosco para o limpador
        GLfloat arm_ambient[] = {0.02f, 0.02f, 0.02f, 1.0f};
        GLfloat arm_diffuse[] = {0.02f, 0.02f, 0.02f, 1.0f};
        GLfloat arm_specular[] = {0.05f, 0.05f, 0.05f, 1.0f};
        GLfloat arm_shininess = 5.0f;

        glMaterialfv(GL_FRONT, GL_AMBIENT, arm_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, arm_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, arm_specular);
        glMaterialf(GL_FRONT, GL_SHININESS, arm_shininess);

        // Haste esquerda
        glPushMatrix();
            glTranslatef(-3.0f, -2.0f, 0.0f);
            glScalef(0.5f, 4.0f, 0.5f);
            glutSolidCube(1.0f);
        glPopMatrix();

        // Haste direita
        glPushMatrix();
            glTranslatef(3.0f, -2.0f, 0.0f);
            glScalef(0.5f, 4.0f, 0.5f);
            glutSolidCube(1.0f);
        glPopMatrix();

        // Barra que empurra
        glPushMatrix();
            glTranslatef(0.0f, -4.0f, 0.0f);
            glScalef(7.0f, 0.8f, 0.5f);
            glutSolidCube(1.0f);
        glPopMatrix();

    glPopMatrix();
}

// Detecta colisões entre a bola e os pinos
void detectarColisoes() {
    for (int i = 0; i < NUM_PINOS; ++i) {
        if (pinos[i].derrubado) continue;

        float dx = posicaoBolaX - pinos[i].x;
        float dz = posicaoBolaZ - pinos[i].z;
        float distancia = std::sqrt(dx * dx + dz * dz);

        const float D_MIN = 0.45f;

        if (distancia <= D_MIN) {
            if (!pinos[i].derrubado) {
                pinos[i].derrubado = true;
                pinos[i].anguloAlvo = 90.0f;
                pinos[i].anguloAtual = 0.0f;
                std::cout << "Pino " << i + 1 << " derrubado por colisao!" << std::endl;
            }
        }
    }
}

// Atualiza a lógica do jogo
void atualizarLogicaJogo(float deltaTime) {
    // Movimento da bola se lançada
    if (jogoLancado) {
        posicaoBolaZ -= velocidadeBola * deltaTime;

        // Atrito simples
        velocidadeBola *= 0.995f;

        // Rotação proporcional à velocidade
        rotacaoBola += velocidadeBola * 100.0f * deltaTime;

        detectarColisoes();

        if (posicaoBolaZ < -10.0f || velocidadeBola < 0.01f) {
            jogoLancado = false;
            std::cout << "Turno encerrado. Pressione R para resetar (ou ative o limpador)." << std::endl;
        }
    }

    // Animação de queda dos pinos
    for (int i = 0; i < NUM_PINOS; ++i) {
        if (pinos[i].derrubado && pinos[i].anguloAtual < pinos[i].anguloAlvo) {
            pinos[i].anguloAtual += 200.0f * deltaTime;
            if (pinos[i].anguloAtual > pinos[i].anguloAlvo) pinos[i].anguloAtual = pinos[i].anguloAlvo;
        }
    }

    // Sweep (limpador)
    if (limpando) {
        const float velocidade = 120.0f;
        if (faseLimpador == 1) { // descendo
            anguloLimpador -= velocidade * deltaTime;
            if (anguloLimpador <= 0.0f) {
                anguloLimpador = 0.0f;
                faseLimpador = 2;
                tempoLimpador = 0.0f;
            }
        } else if (faseLimpador == 2) { // limpando
            for (int i = 0; i < NUM_PINOS; ++i) {
                if (!pinos[i].derrubado) {
                    pinos[i].derrubado = true;
                    pinos[i].anguloAtual = 0.0f;
                    pinos[i].anguloAlvo = 90.0f;
                }
            }
            tempoLimpador += deltaTime;
            if (tempoLimpador >= 1.0f) faseLimpador = 3;
        } else if (faseLimpador == 3) { // subindo
            anguloLimpador += velocidade * deltaTime;
            if (anguloLimpador >= 90.0f) {
                anguloLimpador = 90.0f;
                limpando = false;
                faseLimpador = 0;
                tempoLimpador = 0.0f;
                reiniciarJogo(); // reinicia pinos e bola
            }
        }
    }
}

// timer callback para animação
void timer(int value) {
    float deltaTime = 0.016f; // ~60 FPS
    atualizarLogicaJogo(deltaTime);
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Reinicia o jogo
void reiniciarJogo() {
    inicializarPinos();
    posicaoBolaX = 0.0f;
    posicaoBolaZ = 10.0f;
    velocidadeBola = 0.0f;
    rotacaoBola = 0.0f;
    jogoLancado = false;

    anguloLimpador = 90.0f;
    limpando = false;
    faseLimpador = 0;
    tempoLimpador = 0.0f;

    std::cout << "\nJogo reiniciado. Posicione a bola e pressione ESPACO para lancar." << std::endl;
}

// Teclas normais
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ' ': // Lançar a bola
            if (!jogoLancado) {
                velocidadeBola;
                jogoLancado = true;
                std::cout << "Bola lancada com velocidade " << velocidadeBola << std::endl;
            }
            break;
        case 'r':
        case 'R':
            if (!limpando) {
                limpando = true;
                faseLimpador = 1; // descer
                tempoLimpador = 0.0f;
                std::cout << "Limpador ativado (R)." << std::endl;
            }
            break;
        case 27: // ESC
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// Teclas especiais (setas)
void specialKeys(int key, int x, int y) {
    const float passo = 0.1f;

    if (!jogoLancado) {
        switch (key) {
            case GLUT_KEY_LEFT:
                posicaoBolaX -= passo;
                if (posicaoBolaX < -0.7f) posicaoBolaX = -0.7f;
                break;
            case GLUT_KEY_RIGHT:
                posicaoBolaX += passo;
                if (posicaoBolaX > 0.7f) posicaoBolaX = 0.7f;
                break;
            case GLUT_KEY_UP:
                velocidadeBola += 1.0f;
                if (velocidadeBola > 30.0f) velocidadeBola = 30.0f;
                std::cout << "Velocidade (pre-lancamento) = " << velocidadeBola << std::endl;
                break;
            case GLUT_KEY_DOWN:
                velocidadeBola -= 1.0f;
                if (velocidadeBola < 0.0f) velocidadeBola = 0.0f;
                std::cout << "Velocidade (pre-lancamento) = " << velocidadeBola << std::endl;
                break;
        }
    }
    glutPostRedisplay();
}

// Renderização da cena (display)
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Configuração da câmera
    gluLookAt(camX, camY, camZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);

    // Desenha pista
    desenharPista();

    // Desenha bola
    desenharBola(posicaoBolaX, 0.3f, posicaoBolaZ);

    // Desenha pinos
    for (int i = 0; i < NUM_PINOS; ++i) {
        desenharPino(pinos[i]);
    }

    // Desenha limpador (sweeper)
    desenharLimpador();

    glutSwapBuffers();
}

// Função principal
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Simulador de Boliche");

    init(); // Inicializa OpenGL e cena

    // Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, timer, 0);

    // Documentação dos controles
    std::cout << "Controles: " << std::endl;
    std::cout << "Setas Esquerda/Direita: mover a bola lateralmente (pre-lancamento)." << std::endl;
    std::cout << "Setas Cima/Baixo: ajustar velocidade (pre-lancamento)." << std::endl;
    std::cout << "ESPACO: lancar a bola." << std::endl;
    std::cout << "R: ativar limpador (varre os pinos e reinicia o jogo)." << std::endl;
    std::cout << "ESC: sair." << std::endl;

    glutMainLoop();
    return 0;
}

