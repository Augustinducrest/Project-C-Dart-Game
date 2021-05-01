/*****************************************************************************\
 * TP CPE, 4ETI, TP synthese d'images
 * --------------
 * Charlie Gat
 * Augustin du Crest de Villeneuve
 * Programme principal des appels OpenGL
 \*****************************************************************************/

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <string>


#define GLEW_STATIC 1
#include <GL/glew.h>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#define __gl_h_
#include <GLUT/glut.h>
#define MACOSX_COMPATIBILITY GLUT_3_2_CORE_PROFILE
#else
#include <GL/glut.h>
#define MACOSX_COMPATIBILITY 0
#endif

#include "glhelper.hpp"
#include "mat4.hpp"
#include "vec3.hpp"
#include "vec2.hpp"
#include "triangle_index.hpp"
#include "vertex_opengl.hpp"
#include "image.hpp"
#include "mesh.hpp"

/*****************************************************************************\
 * Variables globales
 *
 *
 \*****************************************************************************/

//identifiant du shader
GLuint shader_program_id;

//Matrice de transformation
struct transformation
{
  mat4 rotation;
  vec3 rotation_center;
  vec3 translation;

  transformation():rotation(),rotation_center(),translation(){}
};

//Informations correspondants à un model
struct model
{
  GLuint vao = 0;
  GLuint texture_id=0;
  int nbr_triangle;
  transformation transformation_model;
  vec3 angle = vec3(0.0f,0.0f,0.0f); // angle suivant x, y et z
  // rotation autour de y non implémentée
};


struct text {
    GLuint vao = 0;             // Vertex array id
    GLuint vbo = 0;             // Vertex buffer id
    GLuint vboi = 0;             // Index buffer id
    GLuint texture_id = 0;       // Well, texture id...
    std::string value;           // Value of the text to display
    transformation transform;    // Rotation & translation
    text() :transform() {}
};
text text_to_draw;
text jouer;
text higher_score;
text text_tentatives;


//Liste des modèls
model model_fleche;
model model_cible;
model model_sol;
model model_fond;
model curseur;

//Transformation de la vue (camera)
transformation transformation_view;
float angle_view = 0.0f;

//Matrice de projection
mat4 projection;

//var_globales
float depla = 0.1f;
int best_score = 0;
float offset_cible_y = 1.0f;
float offset_cible_x = 0.0f;
float angle_tirx = 0.0f;
float angle_tiry = 0.0f;
int score = 0;
float pos = 0.0f;
float temps = 0.0f;
float v_init = 2.0f;
int sens = 0;
bool jeu = false;
int tentatives = 5;


//prototypes

void load_texture(const char* filename,GLuint *texture_id);

void init_model_fleche();
void init_model_sol();
void init_model_cible();
void init_model_fond();
void init_curseur();
void init_text(text*);
void draw_texts(text*);
void lancer(int);

void draw_model1(model model_id);


static void init()
{

    // Chargement du shader
    shader_program_id = glhelper::create_program_from_file(
        "shaders/shader.vert",
        "shaders/shader.frag"); CHECK_GL_ERROR();
    glUseProgram(shader_program_id);

    //matrice de projection
    projection = matrice_projection(60.0f * M_PI / 180.0f, 1.0f, 0.01f, 100.0f);
    GLint loc_projection = glGetUniformLocation(shader_program_id, "projection"); CHECK_GL_ERROR();
    if (loc_projection == -1) std::cerr << "Pas de variable uniforme : projection" << std::endl;
    glUniformMatrix4fv(loc_projection, 1, false, pointeur(projection)); CHECK_GL_ERROR();

    //centre de rotation de la 'camera' (les objets sont centres en z=-2)
    transformation_view.rotation_center = vec3(0.0f, 0.0f, -2.0f);

    //activation de la gestion de la profondeur
    glEnable(GL_DEPTH_TEST); CHECK_GL_ERROR();

    // Charge modeles sur la carte graphique (et leurs valeurs initiales pour les textes)
    init_model_fleche();
    init_model_sol();
    init_model_cible();
    init_model_fond();
    init_curseur();
    text_to_draw.value = "Score : 0";
    init_text(&text_to_draw);
    jouer.value = "Appuyez sur F1 pour jouer.";
    init_text(&jouer);
    higher_score.value = "Higher_Score : 0";
    init_text(&higher_score);
    text_tentatives.value = "Tentatives : 5";
    init_text(&text_tentatives);

    //deplacements initiaux

    //deplacement position depart boule
    model_fleche.transformation_model.translation.y += 0.5f;
    //deplacement position cible
    model_cible.transformation_model.translation.y+= offset_cible_y;
    model_cible.transformation_model.translation.z-=10;
    //deplacement position curseur
    curseur.transformation_model.translation.x = 0.7;
    curseur.transformation_model.translation.y = 1.0;
    //deplacement score
    text_to_draw.transform.translation.z = -12.0;
    text_to_draw.transform.translation.x = -2.0;
    text_to_draw.transform.translation.y = 2.0;
    //deplacement h_score
    higher_score.transform.translation.z = -12.0;
    higher_score.transform.translation.x = -6.0;
    higher_score.transform.translation.y = -2.0;
    //deplacement jouer
    jouer.transform.translation.z = -12.0;
    jouer.transform.translation.x = -6.0;
    //deplacement tentatives
    text_tentatives.transform.translation.z = -12.0;
    text_tentatives.transform.translation.x = -6.0;
    text_tentatives.transform.translation.y = +4.0;
    //deplacement fond
    
    model_fond.transformation_model.translation.z = -15.0;
    model_fond.transformation_model.rotation_center = vec3(0.0f, 0.0f, -15.0f);
    model_fond.transformation_model.rotation = matrice_rotation(4.7, 1.0f, 0.0f, 0.0f);
    
    //deplacement caméra
    transformation_view.rotation = matrice_rotation(angle_view, 0.0f, 1.0f, 0.0f);
}

//Fonction d'affichage
static void display_callback()
{
  //effacement des couleurs du fond d'ecran
  glClearColor(0.5f, 0.6f, 0.9f, 1.0f); CHECK_GL_ERROR();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK_GL_ERROR();

  glDrawArrays(GL_TRIANGLES, 0, 3);

  //défini le programme à utiliser
  glUseProgram(shader_program_id);

  // Affecte les parametres uniformes de la vue (identique pour tous les modeles de la scene)
  {

    //envoie de la rotation
    GLint loc_rotation_view = glGetUniformLocation(shader_program_id, "rotation_view"); CHECK_GL_ERROR();
    if (loc_rotation_view == -1) std::cerr << "Pas de variable uniforme : rotation_view" << std::endl;
    glUniformMatrix4fv(loc_rotation_view,1,false,pointeur(transformation_view.rotation)); CHECK_GL_ERROR();

    //envoie du centre de rotation
    vec3 cv = transformation_view.rotation_center;
    GLint loc_rotation_center_view = glGetUniformLocation(shader_program_id, "rotation_center_view"); CHECK_GL_ERROR();
    if (loc_rotation_center_view == -1) std::cerr << "Pas de variable uniforme : rotation_center_view" << std::endl;
    glUniform4f(loc_rotation_center_view , cv.x,cv.y,cv.z , 0.0f); CHECK_GL_ERROR();

    //envoie de la translation
    vec3 tv = transformation_view.translation;
    GLint loc_translation_view = glGetUniformLocation(shader_program_id, "translation_view"); CHECK_GL_ERROR();
    if (loc_translation_view == -1) std::cerr << "Pas de variable uniforme : translation_view" << std::endl;
    glUniform4f(loc_translation_view , tv.x,tv.y,tv.z , 0.0f); CHECK_GL_ERROR();
  }
  if (jeu == false) {//si le jeu n'a pas commencé
      draw_texts(&jouer);//affiche consignes et meilleur score
      draw_texts(&higher_score);
  }
  if (jeu == true) { //seulement si le jeu a démarré
      // Affiche les modèles objets
      draw_model1(model_fleche);
      draw_model1(model_cible);
      draw_model1(curseur);
      draw_model1(model_sol);
      draw_model1(model_fond);
      //affiche les textes
      draw_texts(&text_to_draw);
      draw_texts(&text_tentatives);
  }
 
  //Changement de buffer d'affichage pour eviter un effet de scintillement
  glutSwapBuffers();
}

/*****************************************************************************\
 * keyboard_callback                                                         *
 \*****************************************************************************/
static void keyboard_callback(unsigned char key, int, int)
{
  //quitte le programme si on appuie sur les touches 'q', 'Q', ou 'echap'
  //enregistre l'image si on appuie sur la touche 'p'
  switch (key)
  {
    case 'p':
      glhelper::print_screen();
      break;

    case 'q':
    case 'Q':
    case 27:
      exit(0);
      break;

      //gère le lancer avec la barre espace
    case ' ':
        temps = 0.0f;
        lancer(0);
        break;
    case 'a':
        if (sens == 0) {
            v_init += 0.1f;
            curseur.transformation_model.translation.y = v_init-1.0;
            if (v_init >= 3.0f) {
                sens = 1;
            }
        }
        if (sens == 1) {
            v_init -= 0.1f;
            curseur.transformation_model.translation.y = v_init-1.0;
            if (v_init <= 1.5f) {
                sens = 0;
            }
        }
        break;
  }

  model_fleche.transformation_model.rotation = matrice_rotation(model_fleche.angle.y, 0.0f,1.0f,0.0f) * matrice_rotation(model_fleche.angle.x, 1.0f,0.0f,0.0f);
  
  // Des exemples de camera vous sont données dans le programme 10

}

/*****************************************************************************\
 * special_callback                                                          *
 \*****************************************************************************/
static void special_callback(int key, int,int)
{
  float dL=0.008f;
  switch (key)
  {
    case GLUT_KEY_F1:
        jeu = true;
        tentatives = 5;
        init_text(&text_tentatives);
        score = 0;
        init_text(&text_to_draw);
        //Fonction de gestion du clavier
        glutKeyboardFunc(keyboard_callback);
        break;
    case GLUT_KEY_UP://rotation avec la touche du haut
        transformation_view.rotation = matrice_rotation(angle_tirx, 0.0f, 1.0f, 0.0f) * matrice_rotation(-angle_tiry, 1.0f, 0.0f, 0.0f);
        angle_tiry += dL;
      break;
    case GLUT_KEY_DOWN: //rotation avec la touche du bas
        transformation_view.rotation = matrice_rotation(angle_tirx, 0.0f, 1.0f, 0.0f) * matrice_rotation(-angle_tiry, 1.0f, 0.0f, 0.0f);
        angle_tiry -= dL;
      break;
    case GLUT_KEY_LEFT://rotation avec la touche de gauche
        transformation_view.rotation = matrice_rotation(angle_tirx, 0.0f, 1.0f, 0.0f) * matrice_rotation(-angle_tiry, 1.0f, 0.0f, 0.0f);
        angle_tirx -= dL;
      break;
    case GLUT_KEY_RIGHT://rotation avec la touche de droite
        transformation_view.rotation = matrice_rotation(angle_tirx, 0.0f, 1.0f, 0.0f)*matrice_rotation(-angle_tiry, 1.0f, 0.0f, 0.0f);
        angle_tirx += dL;
      break;
  }
}




void lancer(int) {

    //deplacement de la flèche et de la caméra avec gravité
    transformation_view.rotation = matrice_rotation(angle_view, 0.0f, 1.0f, 0.0f);

    temps += 0.25f;

    float dZ = -v_init*cos(angle_tiry)*temps;
    model_fleche.transformation_model.translation.z = dZ;
    transformation_view.translation.z = -dZ;

    model_fleche.transformation_model.translation.x += angle_tirx;
    transformation_view.translation.x -= angle_tirx;

    float dY= -0.04 * temps * temps + (float)v_init * (float)sin(angle_tiry)*temps + 0.5f;
    model_fleche.transformation_model.translation.y = dY;
    transformation_view.translation.y = -dY+0.5f;

    glutSpecialFunc(NULL);//arret temporaire du clavier
    glutKeyboardFunc(NULL);
    //reactualisation de l'affichage
    glutPostRedisplay();


    //en fait on va faire une collision avec un disque plus simple et prendre en compte le fond de la pièce
    pos = (model_fleche.transformation_model.translation.y - offset_cible_y -0.5) * (model_fleche.transformation_model.translation.y- offset_cible_y -0.5) + (model_fleche.transformation_model.translation.x- model_cible.transformation_model.translation.x) * (model_fleche.transformation_model.translation.x- model_cible.transformation_model.translation.x);
    if(((pos<1.0)&(-model_fleche.transformation_model.translation.z >10.0))|(-model_fleche.transformation_model.translation.z >20.0)|(model_fleche.transformation_model.translation.y <=0.0f))
    {
        //conditions points
        if (((-model_fleche.transformation_model.translation.z > 20.0) | (model_fleche.transformation_model.translation.y <= 0.0f))==false) {
            printf("la");
            if ((pos >= 0) & (pos < 0.05)) {
                score += 100;
            }
            if ((pos >= 0.05) & (pos < 0.20)) {
                score += 50;
            }
            if ((pos >= 0.20) & (pos < 1.0)) {
                score += 25;
            }
        }

        //met à jour le score 
        std::string query("Score = " +
            std::to_string(score));
        text_to_draw.value = query;
        init_text(&text_to_draw);
        
        //remet les positions initiales
        transformation_view.translation.z = 0.0f;
        transformation_view.translation.y = 0.0f;
        transformation_view.translation.x = 0.0f;

        model_fleche.transformation_model.translation.z = 0.0f;
        model_fleche.transformation_model.translation.y = 0.5f;
        model_fleche.transformation_model.translation.x = 0.0f;

        //on reinitialise les paramètres
        angle_tirx = 0.0f;
        angle_tiry = 0.0f;
        v_init = 2.0f;
        sens = 0;
        curseur.transformation_model.translation.y = v_init-1.0;
        glutKeyboardFunc(keyboard_callback);
        glutSpecialFunc(special_callback);

        //on réduit le nombre de tentatives
        tentatives -= 1;
        std::string oc("Tentatives : " +
            std::to_string(tentatives));
        text_tentatives.value = oc;
        init_text(&text_tentatives);
        //mode pro : on change la position de départ de la cible aleatoirement
        float kx = rand() % 3 - 1;
        offset_cible_x = kx*1.5 ;
        printf("%f", offset_cible_x);
        model_cible.transformation_model.translation.x = offset_cible_x;
        //finit le jeu s'il n'ya plus de tentatives
        if (tentatives == 0) {
            jeu = false; 
            if (score > best_score) { //gere le meilleur score
                best_score = score;
                printf("%d", best_score);
                std::string ac("Higher_Score : " +
                    std::to_string(best_score));
                higher_score.value = ac;
                init_text(&higher_score);
            }
            glutKeyboardFunc(NULL);//arret clavier
        }
    }
    else {
        //demande de rappel de cette fonction dans 25ms
        glutTimerFunc(25, lancer, 0);
    }
       
}

/*****************************************************************************\
 * timer_callback                                                            *
 \*****************************************************************************/
static void timer_callback(int)
{
    if (jeu == true) {
        model_cible.transformation_model.translation.x += depla;
        if (model_cible.transformation_model.translation.x > 1.0 + offset_cible_x) {
            depla = -depla;
        }
        if (model_cible.transformation_model.translation.x < -1.0 + offset_cible_x) {
            depla = -depla;
        }
   }

  //demande de rappel de cette fonction dans 25ms
  glutTimerFunc(25, timer_callback, 0);

  //reactualisation de l'affichage
  glutPostRedisplay();


}

int main(int argc, char** argv)
{
  //**********************************************//
  //Lancement des fonctions principales de GLUT
  //**********************************************//

  //initialisation
  glutInit(&argc, argv);

  //Mode d'affichage (couleur, gestion de profondeur, ...)
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | MACOSX_COMPATIBILITY);

  //Taille de la fenetre a l'ouverture
  glutInitWindowSize(600, 600);

  //Titre de la fenetre
  glutCreateWindow("OpenGL");

  //Fonction de la boucle d'affichage
  glutDisplayFunc(display_callback);


  //Fonction des touches speciales du clavier (fleches directionnelles)
  glutSpecialFunc(special_callback);

  //Fonction d'appel d'affichage en chaine
  glutTimerFunc(25, timer_callback, 0);

  //Option de compatibilité
  glewExperimental = true;

  //Initialisation des fonctions OpenGL
  glewInit();

  //Affiche la version openGL utilisée
  std::cout << "OpenGL: " << (GLchar *)(glGetString(GL_VERSION)) << std::endl;

  //Notre fonction d'initialisation des donnees et chargement des shaders
  init();


  //Lancement de la boucle (infinie) d'affichage de la fenetre
  glutMainLoop();


  //Plus rien n'est execute apres cela
  return 0;
}

void draw_model1(model m)
{

  //envoie des parametres uniformes
  {
    GLint loc_rotation_model = glGetUniformLocation(shader_program_id, "rotation_model"); CHECK_GL_ERROR();
    if (loc_rotation_model == -1) std::cerr << "Pas de variable uniforme : rotation_model" << std::endl;
    glUniformMatrix4fv(loc_rotation_model,1,false,pointeur(m.transformation_model.rotation));    CHECK_GL_ERROR();

    vec3 c = m.transformation_model.rotation_center;
    GLint loc_rotation_center_model = glGetUniformLocation(shader_program_id, "rotation_center_model"); CHECK_GL_ERROR();
    if (loc_rotation_center_model == -1) std::cerr << "Pas de variable uniforme : rotation_center_model" << std::endl;
    glUniform4f(loc_rotation_center_model , c.x,c.y,c.z , 0.0f);                                 CHECK_GL_ERROR();

    vec3 t = m.transformation_model.translation;
    GLint loc_translation_model = glGetUniformLocation(shader_program_id, "translation_model"); CHECK_GL_ERROR();
    if (loc_translation_model == -1) std::cerr << "Pas de variable uniforme : translation_model" << std::endl;
    glUniform4f(loc_translation_model , t.x,t.y,t.z , 0.0f);                                     CHECK_GL_ERROR();
  }

  glBindVertexArray(m.vao);CHECK_GL_ERROR();

  //affichage
  {
    glBindTexture(GL_TEXTURE_2D, m.texture_id);                             CHECK_GL_ERROR();
    glDrawElements(GL_TRIANGLES, 3*m.nbr_triangle, GL_UNSIGNED_INT, 0);     CHECK_GL_ERROR();
  }
}

void draw_texts(text* text_t) {
    //Send uniforma parameters
    {
        GLint loc_rotation_model = glGetUniformLocation(shader_program_id, "rotation_model"); CHECK_GL_ERROR();
        if (loc_rotation_model == -1) std::cerr << "Pas de variable uniforme : rotation_model" << std::endl;
        glUniformMatrix4fv(loc_rotation_model, 1, false, pointeur(text_t->transform.rotation));    CHECK_GL_ERROR();

        vec3 c = text_t->transform.rotation_center;
        GLint loc_rotation_center_model = glGetUniformLocation(shader_program_id, "rotation_center_model"); CHECK_GL_ERROR();
        if (loc_rotation_center_model == -1) std::cerr << "Pas de variable uniforme : rotation_center_model" << std::endl;
        glUniform4f(loc_rotation_center_model, c.x, c.y, c.z, 0.0f);                                 CHECK_GL_ERROR();

        vec3 t = text_t->transform.translation;
        GLint loc_translation_model = glGetUniformLocation(shader_program_id, "translation_model"); CHECK_GL_ERROR();
        if (loc_translation_model == -1) std::cerr << "Pas de variable uniforme : translation_model" << std::endl;
        glUniform4f(loc_translation_model, t.x, t.y, t.z, 0.0f);                                     CHECK_GL_ERROR();
    }

    glBindVertexArray(text_t->vao); CHECK_GL_ERROR();
    {
        glBindTexture(GL_TEXTURE_2D, text_t->texture_id);                       CHECK_GL_ERROR();
        int nbr_triangles = text_t->value.size() * 2;    // We draw two triangles per char.
        glDrawElements(GL_TRIANGLES, 3 * nbr_triangles, GL_UNSIGNED_INT, 0);               CHECK_GL_ERROR();
    }
}

void init_model_fleche()
{
  // Chargement d'un maillage a partir d'un fichier
  mesh m = load_obj_file("fleche3.obj");

  // Affecte une transformation sur les sommets du maillage
  float s = 0.2f;
  mat4 transform = mat4(   s, 0.0f, 0.0f, 0.0f,
      0.0f,    s, 0.0f,-0.9f,
      0.0f, 0.0f,   s ,-2.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  apply_deformation(&m,transform);

  // Centre la rotation du modele 1 autour de son centre de gravite approximatif
  model_fleche.transformation_model.rotation_center = vec3(0.0f,-0.5f,-2.0f);

  // Calcul automatique des normales du maillage
  update_normals(&m);
  // Les sommets sont affectes a une couleur blanche
  fill_color(&m,vec3(1.0f,1.0f,1.0f));

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_fleche.vao);
  glBindVertexArray(model_fleche.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo); CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo); CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,m.vertex.size()*sizeof(vertex_opengl),&m.vertex[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi); CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi); CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.connectivity.size()*sizeof(triangle_index),&m.connectivity[0],GL_STATIC_DRAW); CHECK_GL_ERROR();

  // Nombre de triangles de l'objet 1
  model_fleche.nbr_triangle = m.connectivity.size();

  // Chargement de la texture
  load_texture("data/stegosaurus.tga",&model_fleche.texture_id);
}

void init_model_sol()
{
  //Creation manuelle du model 2

  //coordonnees geometriques des sommets
  vec3 p0=vec3(-25.0f,-0.9f,-25.0f);
  vec3 p1=vec3( 25.0f,-0.9f,-25.0f);
  vec3 p2=vec3( 25.0f,-0.9f, 25.0f);
  vec3 p3=vec3(-25.0f,-0.9f, 25.0f);

  //normales pour chaque sommet
  vec3 n0=vec3(0.0f,1.0f,0.0f);
  vec3 n1=n0;
  vec3 n2=n0;
  vec3 n3=n0;

  //couleur pour chaque sommet
  vec3 c0=vec3(1.0f,1.0f,1.0f);
  vec3 c1=c0;
  vec3 c2=c0;
  vec3 c3=c0;

  //texture du sommet
  vec2 t0=vec2(0.0f,0.0f);
  vec2 t1=vec2(1.0f,0.0f);
  vec2 t2=vec2(1.0f,1.0f);
  vec2 t3=vec2(0.0f,1.0f);

  vertex_opengl v0=vertex_opengl(p0,n0,c0,t0);
  vertex_opengl v1=vertex_opengl(p1,n1,c1,t1);
  vertex_opengl v2=vertex_opengl(p2,n2,c2,t2);
  vertex_opengl v3=vertex_opengl(p3,n3,c3,t3);

  //tableau entrelacant coordonnees-normales
  vertex_opengl geometrie[]={v0,v1,v2,v3};

  //indice des triangles
  triangle_index tri0=triangle_index(0,1,2);
  triangle_index tri1=triangle_index(0,2,3);
  triangle_index index[]={tri0,tri1};
  model_sol.nbr_triangle = 2;

  //attribution d'une liste d'état (1 indique la création d'une seule liste)
  glGenVertexArrays(1, &model_sol.vao);
  glBindVertexArray(model_sol.vao);

  GLuint vbo;
  //attribution d'un buffer de donnees (1 indique la création d'un buffer)
  glGenBuffers(1,&vbo);                                             CHECK_GL_ERROR();
  //affectation du buffer courant
  glBindBuffer(GL_ARRAY_BUFFER,vbo);                                CHECK_GL_ERROR();
  //copie des donnees des sommets sur la carte graphique
  glBufferData(GL_ARRAY_BUFFER,sizeof(geometrie),geometrie,GL_STATIC_DRAW);  CHECK_GL_ERROR();

  // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(0); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

  // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(1); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

  // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(2); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2*sizeof(vec3))); CHECK_GL_ERROR();

  // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
  glEnableVertexAttribArray(3); CHECK_GL_ERROR();
  // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3*sizeof(vec3))); CHECK_GL_ERROR();

  GLuint vboi;
  //attribution d'un autre buffer de donnees
  glGenBuffers(1,&vboi);                                            CHECK_GL_ERROR();
  //affectation du buffer courant (buffer d'indice)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboi);                       CHECK_GL_ERROR();
  //copie des indices sur la carte graphique
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(index),index,GL_STATIC_DRAW);  CHECK_GL_ERROR();

  // Chargement de la texture
  load_texture("data/grass.tga",&model_sol.texture_id);

}

void init_model_cible()
{
    // Chargement d'un maillage a partir d'un fichier
    mesh m = load_obj_file("cible2.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 0.5f;
    mat4 transform = mat4(s, 0.0f, 0.0f, 0.0f,
        0.0f, s, 0.0f, -0.9f,
        0.0f, 0.0f, s, -2.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    apply_deformation(&m, transform);

    // Centre la rotation du modele 1 autour de son centre de gravite approximatif
    model_cible.transformation_model.rotation_center = vec3(0.0f, -0.5f, -2.0f);

    // Calcul automatique des normales du maillage
    update_normals(&m);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m, vec3(1.0f, 1.0f, 1.0f));

    //attribution d'une liste d'état (1 indique la création d'une seule liste)
    glGenVertexArrays(1, &model_cible.vao);
    glBindVertexArray(model_cible.vao);

    GLuint vbo;
    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1, &vbo); CHECK_GL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER, vbo); CHECK_GL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER, m.vertex.size() * sizeof(vertex_opengl), &m.vertex[0], GL_STATIC_DRAW); CHECK_GL_ERROR();

    // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(0); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

    // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(1); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

    // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(2); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2 * sizeof(vec3))); CHECK_GL_ERROR();

    // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(3); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3 * sizeof(vec3))); CHECK_GL_ERROR();

    GLuint vboi;
    //attribution d'un autre buffer de donnees
    glGenBuffers(1, &vboi); CHECK_GL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi); CHECK_GL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.connectivity.size() * sizeof(triangle_index), &m.connectivity[0], GL_STATIC_DRAW); CHECK_GL_ERROR();

    // Nombre de triangles de l'objet 1
    model_cible.nbr_triangle = m.connectivity.size();

    // Chargement de la texture
    load_texture("data/test6.tga", &model_cible.texture_id);
}

void init_model_fond()
{
    //Creation manuelle du model 2

    //coordonnees geometriques des sommets
    vec3 p0 = vec3(-25.0f, -0.9f, -25.0f);
    vec3 p1 = vec3(25.0f, -0.9f, -25.0f);
    vec3 p2 = vec3(25.0f, -0.9f, 25.0f);
    vec3 p3 = vec3(-25.0f, -0.9f, 25.0f);

    //normales pour chaque sommet
    vec3 n0 = vec3(0.0f, 1.0f, 0.0f);
    vec3 n1 = n0;
    vec3 n2 = n0;
    vec3 n3 = n0;

    //couleur pour chaque sommet
    vec3 c0 = vec3(1.0f, 1.0f, 1.0f);
    vec3 c1 = c0;
    vec3 c2 = c0;
    vec3 c3 = c0;

    //texture du sommet
    vec2 t0 = vec2(0.0f, 0.0f);
    vec2 t1 = vec2(1.0f, 0.0f);
    vec2 t2 = vec2(1.0f, 1.0f);
    vec2 t3 = vec2(0.0f, 1.0f);

    vertex_opengl v0 = vertex_opengl(p0, n0, c0, t0);
    vertex_opengl v1 = vertex_opengl(p1, n1, c1, t1);
    vertex_opengl v2 = vertex_opengl(p2, n2, c2, t2);
    vertex_opengl v3 = vertex_opengl(p3, n3, c3, t3);

    //tableau entrelacant coordonnees-normales
    vertex_opengl geometrie[] = { v0,v1,v2,v3 };

    //indice des triangles
    triangle_index tri0 = triangle_index(0, 1, 2);
    triangle_index tri1 = triangle_index(0, 2, 3);
    triangle_index index[] = { tri0,tri1 };
    model_fond.nbr_triangle = 2;

    //attribution d'une liste d'état (1 indique la création d'une seule liste)
    glGenVertexArrays(1, &model_fond.vao);
    glBindVertexArray(model_fond.vao);

    GLuint vbo;
    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1, &vbo);                                             CHECK_GL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER, vbo);                                CHECK_GL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometrie), geometrie, GL_STATIC_DRAW);  CHECK_GL_ERROR();

    // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(0); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

    // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(1); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

    // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(2); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2 * sizeof(vec3))); CHECK_GL_ERROR();

    // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(3); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3 * sizeof(vec3))); CHECK_GL_ERROR();

    GLuint vboi;
    //attribution d'un autre buffer de donnees
    glGenBuffers(1, &vboi);                                            CHECK_GL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);                       CHECK_GL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);  CHECK_GL_ERROR();

    // Chargement de la texture
    load_texture("data/grass.tga", &model_fond.texture_id);

}


void init_curseur()
{
    // Chargement d'un maillage a partir d'un fichier
    mesh m = load_obj_file("curseur2.obj");

    // Affecte une transformation sur les sommets du maillage
    float s = 0.2f;
    mat4 transform = mat4(s, 0.0f, 0.0f, 0.0f,
        0.0f, s, 0.0f, -0.9f,
        0.0f, 0.0f, s, -2.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    apply_deformation(&m, transform);

    // Centre la rotation du modele 1 autour de son centre de gravite approximatif
    curseur.transformation_model.rotation_center = vec3(0.0f, -0.5f, -2.0f);

    // Calcul automatique des normales du maillage
    update_normals(&m);
    // Les sommets sont affectes a une couleur blanche
    fill_color(&m, vec3(1.0f, 1.0f, 1.0f));

    //attribution d'une liste d'état (1 indique la création d'une seule liste)
    glGenVertexArrays(1, &curseur.vao);
    glBindVertexArray(curseur.vao);

    GLuint vbo;
    //attribution d'un buffer de donnees (1 indique la création d'un buffer)
    glGenBuffers(1, &vbo); CHECK_GL_ERROR();
    //affectation du buffer courant
    glBindBuffer(GL_ARRAY_BUFFER, vbo); CHECK_GL_ERROR();
    //copie des donnees des sommets sur la carte graphique
    glBufferData(GL_ARRAY_BUFFER, m.vertex.size() * sizeof(vertex_opengl), &m.vertex[0], GL_STATIC_DRAW); CHECK_GL_ERROR();

    // Active l'utilisation des données de positions (le 0 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(0); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les positions des sommets
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

    // Active l'utilisation des données de normales (le 1 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(1); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les normales des sommets
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

    // Active l'utilisation des données de couleurs (le 2 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(2); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les couleurs des sommets
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2 * sizeof(vec3))); CHECK_GL_ERROR();

    // Active l'utilisation des données de textures (le 3 correspond à la location dans le vertex shader)
    glEnableVertexAttribArray(3); CHECK_GL_ERROR();
    // Indique comment le buffer courant (dernier vbo "bindé") est utilisé pour les textures des sommets
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3 * sizeof(vec3))); CHECK_GL_ERROR();

    GLuint vboi;
    //attribution d'un autre buffer de donnees
    glGenBuffers(1, &vboi); CHECK_GL_ERROR();
    //affectation du buffer courant (buffer d'indice)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi); CHECK_GL_ERROR();
    //copie des indices sur la carte graphique
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.connectivity.size() * sizeof(triangle_index), &m.connectivity[0], GL_STATIC_DRAW); CHECK_GL_ERROR();

    // Nombre de triangles de l'objet 1
    curseur.nbr_triangle = m.connectivity.size();

    // Chargement de la texture
    load_texture("data/white.tga", &curseur.texture_id);
}

void init_text(text *t) {
    float font_height = 1.0f;
    float font_width = 0.5f;
    int length = t->value.size();           // Number of char to draw
    std::vector<vertex_opengl>  geometrie;  // We need 4 vertices per char
    std::vector<triangle_index> index;       // We need 2 triangles per char

    int ascii_offset = 32;                // ASCII code of 1st char in texture file is 32
    int width = 30;                // 30 char per line in texture file
    float x_tick = 0.0333f;           // .. so 1/30 char horizontally
    float y_tick = 0.2f;              // And 1/5 char vertically

    // Here we go, for each char, we create a rectangle.
    for (int i = 0; i < length; i++) {
        //Vertices coordinates
        vec3 p0 = vec3(i * font_width, 0.0f, 0.0f);
        vec3 p1 = vec3(i * font_width, font_height, 0.0f);
        vec3 p2 = vec3((i + 1) * font_width, font_height, 0.0f);
        vec3 p3 = vec3((i + 1) * font_width, 0.0f, 0.0f);

        //Vertices normal
        vec3 n0 = vec3(0.0f, 0.0f, 1.0f);
        vec3 n1 = n0;
        vec3 n2 = n0;
        vec3 n3 = n0;

        //Vertices color
        vec3 c0 = vec3(1.0f, 1.0f, 1.0f);
        vec3 c1 = c0;
        vec3 c2 = c0;
        vec3 c3 = c0;

        //Vertices texture
        int ascii_code = (int)t->value[i];                    // Current char ASCII value
        int texture_code = ascii_code - ascii_offset;           // Current char index in our texture
        float texture_x = (texture_code % width) * x_tick;         // Current char horizontal position
        float texture_y = (int)(texture_code / width) * y_tick;    // Current char vertical position
        vec2 t0 = vec2(texture_x, texture_y + y_tick);
        vec2 t1 = vec2(texture_x, texture_y);
        vec2 t2 = vec2(texture_x + x_tick, texture_y);
        vec2 t3 = vec2(texture_x + x_tick, texture_y + y_tick);

        geometrie.push_back(vertex_opengl(p0, n0, c0, t0));
        geometrie.push_back(vertex_opengl(p1, n1, c1, t1));
        geometrie.push_back(vertex_opengl(p2, n2, c2, t2));
        geometrie.push_back(vertex_opengl(p3, n3, c3, t3));

        index.push_back(triangle_index(i * 4, i * 4 + 1, i * 4 + 2));
        index.push_back(triangle_index(i * 4, i * 4 + 2, i * 4 + 3));
    }

    glGenVertexArrays(1, &t->vao);                                              CHECK_GL_ERROR();
    glBindVertexArray(t->vao);                                                  CHECK_GL_ERROR();

    glGenBuffers(1, &t->vbo);                                                    CHECK_GL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, t->vbo);                                       CHECK_GL_ERROR();
    glBufferData(GL_ARRAY_BUFFER, geometrie.size() * sizeof(vertex_opengl), &geometrie[0], GL_STATIC_DRAW);   CHECK_GL_ERROR();

    glEnableVertexAttribArray(0); CHECK_GL_ERROR();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), 0); CHECK_GL_ERROR();

    glEnableVertexAttribArray(1); CHECK_GL_ERROR();
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(vertex_opengl), (void*)sizeof(vec3)); CHECK_GL_ERROR();

    glEnableVertexAttribArray(2); CHECK_GL_ERROR();
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(2 * sizeof(vec3))); CHECK_GL_ERROR();

    glEnableVertexAttribArray(3); CHECK_GL_ERROR();
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_opengl), (void*)(3 * sizeof(vec3))); CHECK_GL_ERROR();

    glGenBuffers(1, &t->vboi);                                                   CHECK_GL_ERROR();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->vboi);                              CHECK_GL_ERROR();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(index), &index[0], GL_STATIC_DRAW);   CHECK_GL_ERROR();

    load_texture("data/fontB.tga", &t->texture_id);
}


void load_texture(const char* filename,GLuint *texture_id)
{
  // Chargement d'une texture (seul les textures tga sont supportes)
  Image  *image = image_load_tga(filename); 
  if (image) //verification que l'image est bien chargee
  {

    //Creation d'un identifiant pour la texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); CHECK_GL_ERROR();
    glGenTextures(1, texture_id); CHECK_GL_ERROR();

    //Selection de la texture courante a partir de son identifiant
    glBindTexture(GL_TEXTURE_2D, *texture_id); CHECK_GL_ERROR();

    //Parametres de la texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); CHECK_GL_ERROR();

    //Envoie de l'image en memoire video
    if(image->type==IMAGE_TYPE_RGB){ //image RGB
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data); CHECK_GL_ERROR();}
    else if(image->type==IMAGE_TYPE_RGBA){ //image RGBA
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data); CHECK_GL_ERROR();}
    else{
      std::cout<<"Image type not handled"<<std::endl;}

    delete image;
  }
  else
  {
    std::cerr<<"Erreur chargement de l'image, etes-vous dans le bon repertoire?"<<std::endl;
    abort();
  }

  GLint loc_texture = glGetUniformLocation(shader_program_id, "texture"); CHECK_GL_ERROR();
  if (loc_texture == -1) std::cerr << "Pas de variable uniforme : texture" << std::endl;
  glUniform1i (loc_texture,0); CHECK_GL_ERROR();
}
