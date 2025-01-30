#include "pch.h"
#include "include/ImageLoader.h"
#include "include/RaphEngine.h"

#include "SDL_image.h"

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

	static std::map<const char*, SDL_Surface*> Surfaces;

	if (Surfaces.find(path) == Surfaces.end()) {
		SDL_Surface* surface = IMG_Load(path);
		if (surface == NULL) {
			std::cout << "Error loading image: " << path << std::endl;
			return NULL;
		}
		std::cout << "Loaded image: " << path << std::endl;
		SDL_Surface* RGBAsurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(surface);

		Surfaces.insert(std::pair<const char*, SDL_Surface*>(path, RGBAsurface));
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


	static std::map<const char*, GLuint> Images;
	if (Images.find(path) == Images.end()) {

		SDL_Surface* RGBAsurface = loadSurface(path);
		if (RGBAsurface == NULL) {
			return 0;
		}

		//MirrorImage((unsigned char*)RGBAsurface->pixels, RGBAsurface->w, RGBAsurface->h, 4);
		GLuint textureID;

		glGenTextures(1, &textureID);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Give the image to OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RGBAsurface->w, RGBAsurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, RGBAsurface->pixels);

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

		Images.insert(std::pair<const char*, GLuint>(path, textureID));
	}

	GLuint tex = Images[path];

	return tex;
}

