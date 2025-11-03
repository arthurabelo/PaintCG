# PaintCG

Trabalho prático da disciplina de **Computação Gráfica** (2025.2), ministrada pelo professor Laurindo de Sousa Britto Neto, desenvolvido por Arthur Rabelo de Carvalho.

## Descrição
Este projeto consiste em um aplicativo de pintura utilizando OpenGL, com foco em algoritmos de rasterização e transformações geométricas implementados "na mão", sem uso das funções de alto nível do OpenGL para tais operações. O objetivo é aprofundar o entendimento dos fundamentos de computação gráfica, especialmente rasterização e manipulação de formas geométricas.

## Funcionalidades Implementadas
- Algoritmo de Bresenham para traçado de linhas usando apenas `GL_POINTS`.
- Suporte ao traçado de linhas em todos os octantes (redução e transformação inversa).
- Desenho de quadriláteros a partir de dois pontos (canto superior esquerdo e inferior direito) usando Bresenham.
- Desenho de triângulos a partir de três coordenadas usando Bresenham.
- Desenho de polígonos com quatro ou mais coordenadas usando Bresenham.
- Transformações geométricas: translação, escala, cisalhamento, reflexão e rotação (implementadas manualmente, sem funções prontas do OpenGL).
- Algoritmo de Bresenham para rasterização de circunferências usando apenas `GL_POINTS`.
- Preenchimento de polígonos rasterizados (exceto circunferências).
- Algoritmo Flood Fill com vizinhança 4 para preenchimento de formas.

## Requisitos
- Windows
- [Visual Studio](https://visualstudio.microsoft.com/)
- OpenGL e GLUT instalados/configurados no ambiente de desenvolvimento

## Instalação
1. Clone este repositório:
   ```sh
   git clone https://github.com/arthurabelo/PaintCG.git
   ```
2. Abra o arquivo de solução `PaintCG.sln` no Visual Studio.
3. Certifique-se de que as bibliotecas OpenGL e GLUT estão instaladas e configuradas no seu ambiente.
4. Compile e execute o projeto.

## Uso
- Utilize o mouse e/ou teclado conforme instruções exibidas na interface do programa para desenhar linhas, polígonos, circunferências e aplicar transformações.
- O preenchimento de formas pode ser realizado selecionando a ferramenta de preenchimento
- O Flood Fill pode ser utilizado para preencher áreas fechadas.

## Estrutura do Projeto
- `main.cpp`: Função principal e inicialização do OpenGL/GLUT.
- `rasterizer.cpp/h`: Algoritmos de rasterização (linhas, polígonos, circunferências).
- `shapes.cpp/h`: Manipulação e desenho de formas geométricas.
- `transforms.cpp/h`: Implementação das transformações geométricas.
- `fill.cpp/h`: Algoritmos de preenchimento (scanline, flood fill).
- `clipping.cpp/h`: (Se aplicável) Algoritmos de recorte.

---

> Trabalho acadêmico - Computação Gráfica (2025.2) - Prof. Laurindo
