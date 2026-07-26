// Salve este arquivo como: transformacoes_cv.cpp
// Compile adicionando as flags do OpenCV: `pkg-config --cflags --libs opencv4`

#include <opencv2/opencv.hpp>
#include <cmath>
#include <vector>
#include <algorithm>


#include "comum.h"
#include "imgcore.h"

// O bloco extern "C" garante que o seu código em C puro enxergue essas funções
extern "C" {

// Inclua aqui os headers do seu projeto para que o C++ conheça suas structs
// #include "suas_estruturas.h" 

// ====================================================================================
// 1. HOMOGRAFIA PARA IMAGEM EM TONS DE CINZA
// ====================================================================================
void extrair_regiao_por_ancoras_cv( const ImagemCinza *IMG, ImagemCinza *img, const IndiceMatriz *ancora ) {
    if ( !IMG || !img || !ancora ) return;

    // Coordenadas reais dos 4 cantos (Obrigatório para Homografia)
    float x_A = ancora[0].j, y_A = ancora[0].i; // Superior Esquerdo
    float x_B = ancora[1].j, y_B = ancora[1].i; // Superior Direito
    float x_C = ancora[2].j, y_C = ancora[2].i; // Inferior Direito
    float x_D = ancora[3].j, y_D = ancora[3].i; // Inferior Esquerdo

    // 1. Cálculo das dimensões reais da folha baseadas nas maiores distâncias
    float largura_topo = std::sqrt( std::pow( x_B - x_A, 2 ) + std::pow( y_B - y_A, 2 ) );
    float largura_base = std::sqrt( std::pow( x_C - x_D, 2 ) + std::pow( y_C - y_D, 2 ) );
    int largura = (int)std::round( std::max( largura_topo, largura_base ) );

    float altura_esq = std::sqrt( std::pow( x_D - x_A, 2 ) + std::pow( y_D - y_A, 2 ) );
    float altura_dir = std::sqrt( std::pow( x_C - x_B, 2 ) + std::pow( y_C - y_B, 2 ) );
    int altura = (int)std::round( std::max( altura_esq, altura_dir ) );

    if ( largura <= 0 || altura <= 0 ) return;

    // 2. Transfere a matriz do Vértice (int**) para a matriz do OpenCV (cv::Mat)
    // Usamos CV_8UC1 (1 byte por pixel) para garantir máxima velocidade no warpPerspective
    cv::Mat src( IMG->nrow, IMG->ncol, CV_8UC1 );
    for ( int y = 0; y < IMG->nrow; ++y ) {
        for ( int x = 0; x < IMG->ncol; ++x ) {
            src.at<uchar>( y, x ) = cv::saturate_cast<uchar>( IMG->image[y][x] );
        }
    }

    // 3. Define os pontos de origem (trapézio torto) e destino (retângulo perfeito)
    std::vector<cv::Point2f> pontos_origem = {
        cv::Point2f( x_A, y_A ), cv::Point2f( x_B, y_B ),
        cv::Point2f( x_C, y_C ), cv::Point2f( x_D, y_D )
    };

    std::vector<cv::Point2f> pontos_destino = {
        cv::Point2f( 0, 0 ),                 cv::Point2f( largura - 1, 0 ),
        cv::Point2f( largura - 1, altura - 1 ), cv::Point2f( 0, altura - 1 )
    };

    // 4. A Magia do OpenCV: Calcula a Matriz de Perspectiva (3x3) e aplica o Warp
    cv::Mat matriz_homografia = cv::getPerspectiveTransform( pontos_origem, pontos_destino );
    cv::Mat dst;
    
    // O Scalar(255) garante que qualquer área fora da folha seja preenchida com branco
    cv::warpPerspective( src, dst, matriz_homografia, cv::Size( largura, altura ), 
                         cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar( 255 ) );

    // 5. Aloca a matriz de destino usando as funções nativas do seu sistema
    if ( img->image != NULL ) liberar_matriz_pixels( img->image, img->nrow );
    img->ncol = largura;
    img->nrow = altura;
    img->image = alocar_matriz_pixels( img->nrow, img->ncol );
    
    // Se você estiver compilando em C++, strlcpy não é padrão, usamos snprintf ou strncpy
    snprintf( img->key, sizeof( img->key ), "%s", IMG->key );
    img->max = IMG->max;

    // 6. Devolve o resultado processado para a estrutura do Vértice
    for ( int y = 0; y < altura; ++y ) {
        for ( int x = 0; x < largura; ++x ) {
            img->image[y][x] = dst.at<uchar>( y, x );
        }
    }
}


// ====================================================================================
// 2. HOMOGRAFIA PARA IMAGEM COLORIDA (RGB)
// ====================================================================================
void extrair_regiao_colorida_por_ancoras_cv( const ImagemColorida *IMG, ImagemColorida *img, const IndiceMatriz *ancora ) {
    if ( !IMG || !img || !ancora ) return;

    float x_A = ancora[0].j, y_A = ancora[0].i;
    float x_B = ancora[1].j, y_B = ancora[1].i;
    float x_C = ancora[2].j, y_C = ancora[2].i;
    float x_D = ancora[3].j, y_D = ancora[3].i;

    int largura = (int)std::round( std::max( 
        std::sqrt( std::pow( x_B - x_A, 2 ) + std::pow( y_B - y_A, 2 ) ),
        std::sqrt( std::pow( x_C - x_D, 2 ) + std::pow( y_C - y_D, 2 ) )
    ) );

    int altura = (int)std::round( std::max( 
        std::sqrt( std::pow( x_D - x_A, 2 ) + std::pow( y_D - y_A, 2 ) ),
        std::sqrt( std::pow( x_C - x_B, 2 ) + std::pow( y_C - y_B, 2 ) )
    ) );

    if ( largura <= 0 || altura <= 0 ) return;

    // Usamos CV_8UC3 (3 canais de 8 bits) para tratar o RGB
    cv::Mat src( IMG->nrow, IMG->ncol, CV_8UC3 );
    for ( int y = 0; y < IMG->nrow; ++y ) {
        for ( int x = 0; x < IMG->ncol; ++x ) {
            PixelRGB p = IMG->image[y][x];
            // O OpenCV não se importa se a ordem é RGB ou BGR aqui, ele apenas interpola os 3 canais!
            src.at<cv::Vec3b>( y, x ) = cv::Vec3b( p.r, p.g, p.b );
        }
    }

    std::vector<cv::Point2f> pontos_origem = {
        cv::Point2f( x_A, y_A ), cv::Point2f( x_B, y_B ),
        cv::Point2f( x_C, y_C ), cv::Point2f( x_D, y_D )
    };

    std::vector<cv::Point2f> pontos_destino = {
        cv::Point2f( 0, 0 ),                 cv::Point2f( largura - 1, 0 ),
        cv::Point2f( largura - 1, altura - 1 ), cv::Point2f( 0, altura - 1 )
    };

    cv::Mat matriz_homografia = cv::getPerspectiveTransform( pontos_origem, pontos_destino );
    cv::Mat dst;
    
    // O Scalar(255, 255, 255) garante as bordas brancas perfeitas
    cv::warpPerspective( src, dst, matriz_homografia, cv::Size( largura, altura ), 
                         cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar( 255, 255, 255 ) );

    if ( img->image != NULL ) liberar_matriz_pixels_colorida( img->image, img->nrow );
    img->ncol = largura;
    img->nrow = altura;
    img->image = alocar_matriz_pixels_colorida( img->nrow, img->ncol );
    
    snprintf( img->key, sizeof( img->key ), "%s", IMG->key );
    img->max = IMG->max;

    for ( int y = 0; y < altura; ++y ) {
        for ( int x = 0; x < largura; ++x ) {
            cv::Vec3b p = dst.at<cv::Vec3b>( y, x );
            img->image[y][x].r = p[0];
            img->image[y][x].g = p[1];
            img->image[y][x].b = p[2];
        }
    }
}

double detectar_angulo_inclinacao_cv( const ImagemCinza *IMG ) {
    if ( !IMG || !IMG->image ) return 0.0;

    // 1. Mapeamento para Matriz do OpenCV
    cv::Mat src( IMG->nrow, IMG->ncol, CV_8UC1 );
    for ( int y = 0; y < IMG->nrow; ++y ) {
        for ( int x = 0; x < IMG->ncol; ++x ) {
            src.at<uchar>( y, x ) = cv::saturate_cast<uchar>( IMG->image[y][x] );
        }
    }

    // 2. Filtro Gaussiano: Mata as sujeiras de pixel único (poeira de scanner)
    cv::Mat blurred;
    cv::GaussianBlur( src, blurred, cv::Size( 5, 5 ), 0 );

    // 3. Canny: Extrai os contornos afiados (bordas dos quadradinhos)
    cv::Mat edges;
    cv::Canny( blurred, edges, 50, 150, 3 );

    // 4. Transformada de Hough (Probabilística)
    // Exigimos retas que tenham pelo menos 40 pixels de comprimento para ignorar rabiscos
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP( edges, lines, 1, CV_PI / 180.0, 50, 40, 10 );

    if ( lines.empty() ) return 0.0;

    double soma_angulos = 0.0;
    int cont_linhas = 0;

    // 5. Filtragem Cirúrgica de Direção
    for ( size_t i = 0; i < lines.size(); i++ ) {
        double dx = ( double )( lines[i][2] - lines[i][0] );
        double dy = ( double )( lines[i][3] - lines[i][1] );
        double angulo = std::atan2( dy, dx ) * 180.0 / CV_PI;

        // O gabarito não rotaciona 45 graus no scanner. Ele rotaciona de -15 a +15.
        // Pegamos tudo que é "quase horizontal" ou "quase vertical" e normalizamos para a média.
        if ( std::abs( angulo ) < 15.0 ) {
            soma_angulos += angulo;
            cont_linhas++;
        } else if ( std::abs( angulo - 90.0 ) < 15.0 ) {
            soma_angulos += ( angulo - 90.0 );
            cont_linhas++;
        } else if ( std::abs( angulo + 90.0 ) < 15.0 ) {
            soma_angulos += ( angulo + 90.0 );
            cont_linhas++;
        } else if ( std::abs( angulo - 180.0 ) < 15.0 ) {
            soma_angulos += ( angulo - 180.0 );
            cont_linhas++;
        } else if ( std::abs( angulo + 180.0 ) < 15.0 ) {
            soma_angulos += ( angulo + 180.0 );
            cont_linhas++;
        }
    }

    if ( cont_linhas == 0 ) return 0.0;

    // Retorna a inclinação exata do papel
    return soma_angulos / cont_linhas;
}




// Retorna TRUE (1) se encontrar exatamente as 4 âncoras, ou FALSE (0) caso falhe.
// Retorna 'h' (horizontal), 'v' (vertical) ou 0 caso falhe.
int encontrar_ancoras_cv(const ImagemCinza *IMG, IndiceMatriz *ancora) {
    if (!IMG || !IMG->image || !ancora) return 0;

    cv::Mat src(IMG->nrow, IMG->ncol, CV_8UC1, IMG->image[0]);

    // 1. MAPEAMENTO COM SATURATE_CAST (Ajuste Fino de Contraste)
    // Vamos aumentar o contraste para forçar as âncoras a ficarem bem escuras
    // O saturate_cast garante que o valor (0-255) não sofra overflow/underflow (ex: 260 vira 255)
    cv::Mat ajustada = cv::Mat::zeros(src.size(), src.type());
    double alpha = 1.3; // Ganho de contraste (aumenta a diferença entre claro e escuro)
    int beta = -20;     // Reduz o brilho global (escurece levemente o cinza do papel)

    for (int y = 0; y < src.rows; y++) {
        const uchar* linha_in = src.ptr<uchar>(y); // Ponteiro rápido de leitura (C-style)
        uchar* linha_out = ajustada.ptr<uchar>(y); // Ponteiro rápido de escrita
        for (int x = 0; x < src.cols; x++) {
            linha_out[x] = cv::saturate_cast<uchar>(alpha * linha_in[x] + beta);
        }
    }

    // 2. FILTRO GAUSSIANO
    // Suaviza a textura do papel impresso e remove "ruído de sal e pimenta" do scanner
    cv::Mat borrada;
    cv::GaussianBlur(ajustada, borrada, cv::Size(5, 5), 1.5);

    // 3. CANNY EDGE DETECTION
    // Extrai apenas as bordas afiadas. Gradientes de sombra são completamente ignorados.
    cv::Mat bordas;
    cv::Canny(borrada, bordas, 50, 150, 3);

    // 4. TRANSFORMADA DE HOUGH (HoughLinesP)
    // Encontra os segmentos de reta que formam os lados dos quadradinhos das âncoras
    std::vector<cv::Vec4i> linhas;
    int min_line_length = IMG->ncol * 0.005; // Segmentos muito curtos (ruído) são descartados
    int max_line_gap = 5; // Permite que a reta tenha pequenas interrupções (falha de impressão)

    cv::HoughLinesP(bordas, linhas, 1, CV_PI / 180.0, 30, min_line_length, max_line_gap);

    // 5. RECONSTRUÇÃO GEOMÉTRICA (O Pulo do Gato)
    // Desenhamos apenas as linhas perfeitamente retas encontradas pelo Hough em uma tela limpa
    cv::Mat mascara_linhas = cv::Mat::zeros(bordas.size(), CV_8UC1);
    for (size_t i = 0; i < linhas.size(); i++) {
        cv::Vec4i l = linhas[i];
        cv::line(mascara_linhas, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2, cv::LINE_AA);
    }

    // Fechamento Morfológico: "Solda" as linhas desconectadas para formar quadrados fechados
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(mascara_linhas, mascara_linhas, cv::MORPH_CLOSE, kernel);

    // 6. EXTRAÇÃO DOS CONTORNOS (Agora sobre uma máscara matematicamente perfeita)
    std::vector<std::vector<cv::Point>> contornos;
    cv::findContours(mascara_linhas, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Rect> caixas;
    double min_area = (IMG->ncol * IMG->nrow) * 0.0003;
    double max_area = (IMG->ncol * IMG->nrow) * 0.03;

    for (const auto& contorno : contornos) {
        cv::Rect bbox = cv::boundingRect(contorno);
        double area = bbox.width * bbox.height; // Usamos a área do bounding box diretamente

        if (area < min_area || area > max_area) continue;

        double aspect_ratio = (double)bbox.width / bbox.height;

        // Se for um quadrado (lados quase iguais), consideramos como âncora
        if (aspect_ratio >= 0.70 && aspect_ratio <= 1.30) {
            bool duplicado = false;
            for (const auto& c : caixas) {
                if (std::abs(c.x - bbox.x) < 15 && std::abs(c.y - bbox.y) < 15) {
                    duplicado = true;
                    break;
                }
            }
            if (!duplicado) caixas.push_back(bbox);
        }
    }

    // 7. VALIDAÇÃO
    if (caixas.size() == 3) {
        puts("[AVISO] 3 ancoras encontradas. Restauração matemática necessária.");
        // A lógica de restaurar a 4ª entra aqui
    } else if (caixas.size() != 4) {
        return 0; // Falhou na detecção
    }

    // 8. ORDENAÇÃO ESPACIAL E COORDENADAS REAIS (Sua lógica mantida intacta)
    std::sort(caixas.begin(), caixas.end(), [](const cv::Rect& a, const cv::Rect& b) {
        return (a.y + a.height / 2) < (b.y + b.height / 2);
    });

    if ((caixas[0].x + caixas[0].width / 2) > (caixas[1].x + caixas[1].width / 2))
        std::swap(caixas[0], caixas[1]);

    if ((caixas[2].x + caixas[2].width / 2) < (caixas[3].x + caixas[3].width / 2))
        std::swap(caixas[2], caixas[3]);

    ancora[0].j = caixas[0].x;   ancora[0].i = caixas[0].y;
    ancora[1].j = caixas[1].x + caixas[1].width; ancora[1].i = caixas[1].y;
    ancora[2].j = caixas[2].x + caixas[2].width; ancora[2].i = caixas[2].y + caixas[2].height;
    ancora[3].j = caixas[3].x;   ancora[3].i = caixas[3].y + caixas[3].height;

    // 9. CÁLCULO DE DIREÇÃO
    char direcao = (  ancora[0].i + ancora[1].i - ancora[2].i - ancora[3].i  <
                    - ancora[0].j + ancora[1].j + ancora[2].j - ancora[3].j  ) ? 'h' : 'v';

    return direcao;
}




} // Fim do bloco extern "C"
