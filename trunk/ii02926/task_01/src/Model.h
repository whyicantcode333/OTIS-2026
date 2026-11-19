#pragma once

#include <string>

class Model {
public:
    virtual ~Model() = default;

    // Один шаг симуляции: вход u, выход y
    virtual double next(double u) = 0;

    // Сброс состояния модели
    virtual void reset() = 0;

    // Имя модели для интерфейса и логов
    virtual std::string name() const = 0;

    // Проверка устойчивости (Advanced)
    virtual bool isStable() const { return true; }

    // Текст предупреждения о неустойчивости
    virtual std::string stabilityWarning() const { return ""; }
};