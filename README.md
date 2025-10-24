# PaintCG - Bresenham & Rasterização (exemplo inspirado no GIMP)

Este projeto é um exemplo didático em C++ usando GLUT/OpenGL que implementa técnicas de rasterização fundamentais sem usar transformações prontas do OpenGL. Foi desenvolvido como resposta ao enunciado (em Português) que pede a implementação do Algoritmo de Bresenham (linhas), redução ao primeiro octante e inversa, rasterização de círculos (Bresenham/midpoint), polígonos, triângulos, retângulos, preenchimento por scanline e Flood-Fill (viz.4), além das transformações geométricas (translação, escala, cisalhamento, reflexão e rotação) sem utilizar a API de transformações do OpenGL.

Requisitos/funções implementadas (conforme enunciado):
- a) Algoritmo de Bresenham para traçado de linhas (usando apenas GL_POINTS).
- b) Redução ao primeiro octante + transformação inversa (implementadas como utilitários para entender a técnica; a rotina principal de rasterização usa estes passos).
- c) Desenho de quadriláteros (retângulos) a partir do canto superior esquerdo e inferior direito.
- d) Desenho de triângulos a cada três coordenadas.
- e) Desenho de polígonos com 4+ vértices (fechados).
- f) Transformações geométricas aplicadas aos vértices (translate, scale, shear, reflect, rotate) via multiplicação de matrizes 3x3 composta.
- g) Rasterização de circunferência (algoritmo de midpoint / Bresenham do círculo).
- h) Preenchimento de polígonos usando algoritmo de Scanline fill (varredura).
- i) Flood Fill com vizinhança 4, implementado iterativamente (pilha) para evitar estouro de pilha.

Observações importantes:
- Todos os pixels são desenhados com glBegin(GL_POINTS) / glVertex2i conforme solicitado. A aplicação mantém também uma matriz auxiliar (framebuffer em RAM) com a cor de cada pixel para acelerar operações de preenchimento/checagens (permitido no enunciado).
- As transformações são aplicadas aos vértices, gerando novas coordenadas inteiras antes de rasterizar as arestas novamente — não é utilizada nenhuma função de transformações do OpenGL.
- O projeto é intencionalmente simples e didático; a interface é básica (mouse + teclado). Use o menu e as teclas descritas abaixo.

Compilação:
- Requer GLUT (freeglut) e um compilador C++ moderno (g++ / clang++).
- Exemplo (Linux/macOS):
  g++ -std=c++11 main.cpp -o paintcg -lGL -lGLU -lglut

Como usar:
- Menu do botão direito para escolher modo ou use as teclas:
  - 'l' ou '1': Linha
  - 'r' ou '2': Retângulo
  - 't' ou '3': Triângulo
  - 'p' ou '4': Polígono
  - 'c' ou '5': Círculo
  - 'f' : Preencher polígono (scanline) para última forma desenhada
  - 'o' : Flood fill (click dentro da forma para preencher)
  - 'x' : Apagar tela
  - ESC : Sair

Mouse:
- Clique esquerdo para inserir vértices. Para:
  - Linha: dois cliques.
  - Retângulo: primeiro clique = canto superior esquerdo, segundo clique = canto inferior direito.
  - Triângulo: três cliques.
  - Polígono: clique várias vezes; clique direito para fechar e inserir o polígono.
  - Círculo: clique centro, segundo clique define raio (ponto na circunferência).
- Flood fill: selecione modo 'flood' (tecla 'o') e clique dentro da região a ser preenchida.

Arquivos fornecidos:
- main.cpp : implementação completa (GLUT + algoritmos).
- glut_text.h : helper para desenhar texto (fornecido no enunciado original).

Boa leitura / execução. Se quiser, posso dividir o código em múltiplos módulos ou explicar passo a passo as funções (Bresenham, scanline, flood-fill, transformações), ou adaptar o UI para mais recursos (undo, cores, camadas).