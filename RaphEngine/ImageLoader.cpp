#include "pch.h"
#include "include/ImageLoader.h"
#include "include/RaphEngine.h"

#include <SDL_image.h>

#include <iostream>
#include <map>

static void MirrorImage(unsigned char* pixels, int width, int height, int bytesPerPixels) {
	unsigned char* row = new unsigned char[width * bytesPerPixels];
	for (int i = 0; i < height / 2; i++) {
		unsigned char* row1 = pixels + i * width * bytesPerPixels;
		unsigned char* row2 = pixels + (height - i - 1) * width * bytesPerPixels;
		memcpy(row, row1, width * bytesPerPixels);
		memcpy(row1, row2, width * bytesPerPixels);
		memcpy(row2, row, width * bytesPerPixels);
	}
	delete[] row;
}

SDL_Surface* loadSurface(const char* path) {

	static std::map<std::string, SDL_Surface*> Surfaces;

	if (Surfaces.find(path) == Surfaces.end()) {
		SDL_Surface* surface = IMG_Load(path);
		if (surface == NULL) {
			std::cout << "Error loading image: " << path << std::endl;
			return NULL;
		}
		std::cout << "Loaded image: " << path << std::endl;
		SDL_Surface* RGBAsurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(surface);

		Surfaces.insert(std::pair<std::string, SDL_Surface*>(path, RGBAsurface));
	}

	SDL_Surface* surf = Surfaces[path];

	return surf;
}

GLFWimage* ImageLoader::LoadImageDataGL(const char* path) {
	SDL_Surface* RGBAsurface = loadSurface(path);
	if (RGBAsurface == NULL) {
		return NULL;
	}

	MirrorImage((unsigned char*)RGBAsurface->pixels, RGBAsurface->w, RGBAsurface->h, 4);

	GLFWimage* image = new GLFWimage();
	image->width = RGBAsurface->w;
	image->height = RGBAsurface->h;
	image->pixels = (unsigned char *)RGBAsurface->pixels;
	return image;
}



GLuint ImageLoader::LoadImageGL(const char* path, bool smooth) {


	static std::map<std::string, GLuint> Images;
    if(!path) {
        return 0;
    }

    for (auto& pair : Images) {
        if (pair.first == path) {
            return pair.second;
        }
    }
    SDL_Surface* RGBAsurface = loadSurface(path);
    if (RGBAsurface == NULL) {
        printf("Failed to load image: %s\n", path);
        return 0;
    }


    // get the number of channels in the SDL surface
    GLint nOfColors = RGBAsurface->format->BytesPerPixel;
    GLenum texture_format;
    if (nOfColors == 4)     // contains an alpha channel
    {
        printf("Image %s has an alpha channel\n", path);
        if (RGBAsurface->format->Rmask == 0x000000ff)
                texture_format = GL_RGBA;
        else
                texture_format = GL_BGRA;
    } else if (nOfColors == 3)     // no alpha channel
    {
        printf("Image %s has no alpha channel\n", path);
        if (RGBAsurface->format->Rmask == 0x000000ff)
                texture_format = GL_RGB;
        else
                texture_format = GL_BGR;
    } else {
            printf("warning: the image is not truecolor..  this will probably break\n");
            // this error should not go unhandled
    }


    //MirrorImage((unsigned char*)RGBAsurface->pixels, RGBAsurface->w, RGBAsurface->h, 4);
    GLuint textureID;

    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    printf("Loaded image: %s with id = %d\n", path, textureID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, texture_format, RGBAsurface->w, RGBAsurface->h, 0, texture_format, GL_UNSIGNED_BYTE, RGBAsurface->pixels);

    // OpenGL has now copied the data. Free our own version

    if (smooth) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(RGBAsurface);

    Images.insert(std::pair<std::string, GLuint>(path, textureID));

	return textureID;
}
