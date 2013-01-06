#include <pango/TextSurface.h>

TextSurface::TextSurface() 
  :cr(NULL)
  ,surface(NULL)
  ,layout(NULL)
  ,font_desc(NULL)
  ,width(0)
  ,height(0)
{
}

TextSurface::~TextSurface() {
  if(layout != NULL) {
    g_object_unref(layout);
    layout = NULL;
  }
}

bool TextSurface::setup(const std::string& font, int w, int h) {
  width = w;
  height = h;

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  if(surface == NULL) {
    printf("ERROR: Cannot create surface (%d, %d)\n", width, height);
    return false;
  }

  cr = cairo_create(surface);
  if(cairo_status(cr) == CAIRO_STATUS_NO_MEMORY) {
    printf("ERROR: Cannot create cairo context.\n");
    return false;
  }

  layout = pango_cairo_create_layout(cr);
  if(layout == NULL) {
    printf("ERROR: Cannot pango_cairo_create_layout()\n");
    return false;
  }

  font_desc = pango_font_description_from_string(font.c_str());
  if(font_desc == NULL) {
    printf("ERROR: Cannot pango_font_description_from_string(%s).\n", font.c_str());
    return false;
  }

  pango_layout_set_font_description(layout, font_desc);
  pango_font_description_free(font_desc);
  font_desc = NULL;
  return true;
}

bool TextSurface::setFont(const std::string& font) {
  assert(layout);
  font_desc = pango_font_description_from_string(font.c_str());
  if(font_desc == NULL) {
    printf("ERROR: Cannot pango_font_description_from_string(%s).\n", font.c_str());
    return false;
  }

  pango_layout_set_font_description(layout, font_desc);
  pango_font_description_free(font_desc);
  font_desc = NULL;
  return true;
}

void TextSurface::setText(const std::string& txt, float r, float g, float b, float a) {
  assert(cr);
  assert(layout);
  cairo_set_source_rgba(cr, r, g, b, a);
  pango_layout_set_text(layout, txt.c_str(), -1);
}

void TextSurface::setMarkup(const std::string& txt) {
  assert(layout);
  pango_layout_set_markup(layout, txt.c_str(), txt.size());
}

void TextSurface::render() {
  assert(layout); 
  assert(cr);
  pango_cairo_show_layout(cr, layout);
}

bool TextSurface::saveAsPNG(const std::string& filepath) {
  assert(surface);
  status = cairo_surface_write_to_png(surface, filepath.c_str());
  if(status != CAIRO_STATUS_SUCCESS) {
    printf("ERROR: Cannot saveAsPNG(), to %s\n", filepath.c_str());
    return false;
  }
  return true;
}

void TextSurface::move(double x, double y) {
  assert(cr);
  cairo_move_to(cr, x, y);
}