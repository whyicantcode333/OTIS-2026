#ifndef MODEL_H
#define MODEL_H

class Model {
private:
    double state_value;

public:
    explicit Model(double initial_state = 0.0) : state_value(initial_state) {}
    virtual ~Model() = default;

    virtual double step(double u) = 0;

    double getState() const { return state_value; }
    double getY() const { return state_value; }
    void setY(double new_y) { state_value = new_y; }
    virtual void reset(double initial_state = 0.0) { state_value = initial_state; }
};

#endif