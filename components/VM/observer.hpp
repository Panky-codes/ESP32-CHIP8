#ifndef OBSERVER_HPP_
#define OBSERVER_HPP_

class IObserver {
public:
  virtual void update() = 0;
  virtual ~IObserver() = default;
};

class ExitButton : public IObserver {
public:
  bool isPressed() { return is_exit_pressed; }
  void update() { is_exit_pressed = true; }
  void clearExitFlag() { is_exit_pressed = false; }

private:
  bool is_exit_pressed = false;
};
#endif // OBSERVER_HPP_