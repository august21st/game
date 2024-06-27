#include <godot_cpp/classes/control.hpp>

class LoadingScreen : public godot::Control {
	GDCLASS(LoadingScreen, Control);

protected:
	static void _bind_methods();

public:
	LoadingScreen();
	~LoadingScreen();

	virtual void _ready() override;
};