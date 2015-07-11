namespace ogl
{
	bool		create(const char* title, int width, int height, unsigned char bits=32, bool fullscreen=false);
	void		destroy();
	int			input();
	extern bool	keys[256];	// Array Used For The Keyboard Routine
	bool		load(const char* url, unsigned* texture, int num=1);
}