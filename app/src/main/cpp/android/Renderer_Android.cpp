#include "../core/Renderer.h"
#include <vector>
#include <algorithm>
std::vector<uint32_t> g_pixelBuffer;
int g_width = 256, g_height = 240;
struct Renderer::PIMPL {}; 
Renderer::Renderer() : m_impl(nullptr) {}
Renderer::~Renderer() {}
void Renderer::SetWindowTitle(const char* title) {}
void Renderer::Create(size_t w, size_t h) { g_width=w; g_height=h; g_pixelBuffer.resize(w*h); }
void Renderer::Destroy() { g_pixelBuffer.clear(); }
void Renderer::Clear(const Color4& color) { std::fill(g_pixelBuffer.begin(), g_pixelBuffer.end(), color.argb); }
void Renderer::DrawPixel(int32 x, int32 y, const Color4& color) { if(x>=0 && x<g_width && y>=0 && y<g_height) g_pixelBuffer[y*g_width+x] = color.argb; }
void Renderer::Present() {} 
const uint32_t* GetRawPixelBuffer() { return g_pixelBuffer.data(); }
