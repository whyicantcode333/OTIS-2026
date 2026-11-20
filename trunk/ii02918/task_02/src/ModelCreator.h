#pragma once
#include "DynamicModel.h"
#include "StateDelayModel.h"
#include "SaturationModel.h"
#include "QuadraticDecayODE.h"
#include <memory>
#include <iostream>

class ModelCreator
{
public:
    static std::unique_ptr<DynamicModel> create(int choice)
    {
        if (choice == 1) return makeStateDelay();
        if (choice == 2) return makeSaturation();
        if (choice == 3) return makeQuadratic();

        std::cerr << "Bad model id: " << choice << "\n";
        return nullptr;
    }

private:
    static std::unique_ptr<DynamicModel> makeStateDelay()
    {
        double a1, a2, b; int k;
        std::cout << "  a1 = "; std::cin >> a1;
        std::cout << "  a2 = "; std::cin >> a2;
        std::cout << "  b  = "; std::cin >> b;
        std::cout << "  k  = "; std::cin >> k;
        if (k < 1) k = 1;
        return std::make_unique<StateDelayModel>(a1, a2, b, k);
    }

    static std::unique_ptr<DynamicModel> makeSaturation()
    {
        double a, b, lo, hi;
        std::cout << "  a = "; std::cin >> a;
        std::cout << "  b = "; std::cin >> b;
        std::cout << "  Umin = "; std::cin >> lo;
        std::cout << "  Umax = "; std::cin >> hi;
        return std::make_unique<SaturationModel>(a, b, lo, hi);
    }

    static std::unique_ptr<DynamicModel> makeQuadratic()
    {
        double a1, a2, b, dt;
        std::cout << "  a1 = "; std::cin >> a1;
        std::cout << "  a2 = "; std::cin >> a2;
        std::cout << "  b  = "; std::cin >> b;
        std::cout << "  dt = "; std::cin >> dt;
        return std::make_unique<QuadraticDecayODE>(a1, a2, b, dt);
    }
};