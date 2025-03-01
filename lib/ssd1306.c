#include "ssd1306.h"
#include "font.h"
#include "stdio.h"

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c)
{
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

void ssd1306_config(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command)
{
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
      ssd->i2c_port,
      ssd->address,
      ssd->port_buffer,
      2,
      false);
}

void ssd1306_send_data(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
      ssd->i2c_port,
      ssd->address,
      ssd->ram_buffer,
      ssd->bufsize,
      false);
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value)
{
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

/*
void ssd1306_fill(ssd1306_t *ssd, bool value) {
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint8_t i = 1; i < ssd->bufsize; ++i)
    ssd->ram_buffer[i] = byte;
}*/

void ssd1306_fill(ssd1306_t *ssd, bool value)
{
  // Itera por todas as posições do display
  for (uint8_t y = 0; y < ssd->height; ++y)
  {
    for (uint8_t x = 0; x < ssd->width; ++x)
    {
      ssd1306_pixel(ssd, x, y, value);
    }
  }
}

void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill)
{
  for (uint8_t x = left; x < left + width; ++x)
  {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y)
  {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill)
  {
    for (uint8_t x = left + 1; x < left + width - 1; ++x)
    {
      for (uint8_t y = top + 1; y < top + height - 1; ++y)
      {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  int err = dx - dy;

  while (true)
  {
    ssd1306_pixel(ssd, x0, y0, value); // Desenha o pixel atual

    if (x0 == x1 && y0 == y1)
      break; // Termina quando alcança o ponto final

    int e2 = err * 2;

    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }

    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value)
{
  for (uint8_t x = x0; x <= x1; ++x)
    ssd1306_pixel(ssd, x, y, value);
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value)
{
  for (uint8_t y = y0; y <= y1; ++y)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar um caractere na tela
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
  uint16_t index = 0;

  if (c >= 'A' && c <= 'Z')
  {
    index = (c - 'A' + 11) * 8; // Letras maiúsculas
  }
  else if (c >= 'a' && c <= 'z')
  {
    index = (c - 'a' + 37) * 8; // Letras minúsculas
  }
  else if (c >= '0' && c <= '9')
  {
    index = (c - '0' + 1) * 8; // Números
  }
  else if (c == ':') 
  {
    index = (('z' - 'a' + 37) + 1) * 8; // Símbolo ':'
  }
  else if (c == '-') 
  {
    index = (('z' - 'a' + 37) + 2) * 8; // Símbolo '-'
  }

  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t line = font[index + i];
    for (uint8_t j = 0; j < 8; ++j)
    {
      ssd1306_pixel(ssd, x + i, y + j, (line >> j) & 1);
    }
  }
}

// Função para desenhar uma string na tela
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}

// Função para mapear a coordenada x de entrada para a coordenada x do display
uint8_t map_x_to_display(uint16_t input_x)
{
  if (input_x > 4095)
    input_x = 4095; // Limita o valor máximo
  return (input_x * 107) / 4095 + 10; // Mapeia input_x (0-4095) para x (10-117)
}

// Função para mapear a coordenada y de entrada para a coordenada y do display
uint8_t map_y_to_display(uint16_t input_y)
{
  if (input_y > 4095)
    input_y = 4095; // Limita o valor máximo

  return 50 - ((input_y * 35) / 4095); // Mapeia input_y (0-4095) para y (15-50)
}

// Função para desenhar um mapa mundial na tela
void ssd1306_draw_world_map(ssd1306_t *ssd, const uint8_t *bitmap)
{
  for (uint8_t y = 0; y < 64; ++y)
  {
    for (uint8_t x = 0; x < 128; ++x)
    {
      uint16_t byte_index = (y * 128 + x) / 8; // Índice do byte no array
      uint8_t bit_index = x % 8;               // Posição do bit no byte
      uint8_t pixel = (epd_bitmap_mapa_mundi_pixell[byte_index] >> (7 - bit_index)) & 1;

      ssd1306_pixel(ssd, x, y, pixel);
    }
  }
}

// Função para desenhar uma cruz na tela
void ssd1306_cross(ssd1306_t *ssd, uint8_t x, uint8_t y, uint8_t length, bool value)
{
  ssd1306_hline(ssd, x - length / 2, x + length / 2, y, value);
  ssd1306_hline(ssd, x - length / 2, x + length / 2, y + 1, value);

  ssd1306_vline(ssd, x, y - length / 2, y + length / 2, value);
  ssd1306_vline(ssd, x + 1, y - length / 2, y + length / 2, value);
}

// Função para obter o valor de um pixel do bitmap
bool ssd1306_get_pixel(const uint8_t *bitmap, uint8_t x, uint8_t y)
{
  if (x >= 128 || y >= 64)
  {
    return false; // Fora dos limites do display
  }

  uint16_t byte_index = (y * 128 + x) / 8; // Índice do byte no array
  uint8_t bit_index = x % 8;               // Posição do bit no byte
  return (bitmap[byte_index] >> (7 - bit_index)) & 1;
}

// Função para desenhar uma imagem centralizada na tela
void ssd1306_draw_centered_image(ssd1306_t *ssd, const uint8_t *clock, uint8_t x_offset, uint8_t y_offset)
{
  for (int y = 0; y < 48; y++)
  { // Linhas (altura da imagem)
    for (int x = 0; x < 48; x++)
    { // Colunas (largura da imagem)
      // Calcula o byte e o bit correspondente ao pixel
      uint8_t byte_index = y * 6 + (x / 8);                    // Cada linha tem 6 bytes (48 pixels)
      uint8_t bit_index = 7 - (x % 8);                         // Inverte a ordem dos bits dentro do byte
      uint8_t pixel = (clock[byte_index] >> bit_index) & 0x01; // Extrai o bit

      // Desenha o pixel na posição correta
      ssd1306_pixel(ssd, x_offset + x, y_offset + y, pixel);
    }
  }
}

// Função para selecionar o fuso horário com base nas coordenadas x e y
uint select_fuso(ssd1306_t *ssd, uint8_t x, uint8_t y)
{
  if (y >= 14)
  {
    // Definir os limites dos fusos horários
    const uint8_t fuso_pixels[] = {6, 11, 15, 19, 24, 28, 33, 37, 42, 46,
                                    51, 55, 60, 64, 68, 73, 77, 82, 86,
                                    91, 95, 100, 104, 109, 113, 117, 119}; 
    const uint8_t num_fusos = 24;

    // Limitar o valor de x a 116 (com um pixel de folga)
    if (x > 116) {
      x = 116;
    }

    // Encontrar o fuso correspondente ao valor de x
    uint8_t fuso_index = 0;
    for (uint8_t i = 0; i < num_fusos + 1; ++i)
    {
      if (x >= fuso_pixels[i] && x < fuso_pixels[i + 1])
      {
        fuso_index = i;
        break;
      }
    }
    // Definir os limites do quadrado
    uint8_t left = fuso_pixels[fuso_index];
    uint8_t width = fuso_pixels[fuso_index + 1] - fuso_pixels[fuso_index];
    uint8_t top = 14;       // Ajuste a posição vertical conforme necessário
    uint8_t height = 50;    // Quadrado (largura = altura)

    // Desenhar o quadrado usando a função ssd1306_rect
    ssd1306_rect(ssd, top, left, width, height, false, false);
    
    char buffer[10]; // Buffer para armazenar o número convertido
    // Convertendo o número inteiro "i" para uma string
    if(fuso_index - 12 >= 0) // Se o fuso for maior que 12
      sprintf(buffer, "%d", fuso_index - 12); // Converte "fuso_index" para string
    else // Se for menor que 12
      sprintf(buffer, "-%d", 12 - fuso_index);

    // Agora você pode usar a string com "fuso_index" em ssd1306_draw_string
    ssd1306_draw_string(ssd, "UTC", 0, 40);
    ssd1306_draw_string(ssd, buffer, 0, 51);
    return fuso_index;
  }
}
