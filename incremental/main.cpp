#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <optional>

int main()
{
    // Create the main window (SFML 3.0 syntax)
    sf::RenderWindow window;
    window.create(sf::VideoMode(sf::Vector2u(1280, 720)), "Procedural Civilization - Idle Game");
    window.setFramerateLimit(60);

    // Load a font for displaying text (we'll need to add a font file)
    // sf::Font font;
    // For now, we'll use the default font - you'll want to add a TTF font file later
    // if (!font.openFromFile("arial.ttf")) {
    //     std::cerr << "Error loading font!" << std::endl;
    // }

    // Clock for delta time calculation
    sf::Clock deltaClock;
    sf::Clock fpsClock;
    int frameCount = 0;
    float fps = 0.0f;

    // Game state variables
    float gameTime = 0.0f;
    double food = 0.0;
    double foodPerSecond = 1.0; // Starting production rate

    // Text display setup (we'll add this once we have a font)
    // sf::Text fpsText(font);
    // sf::Text foodText(font);
    // sf::Text timeText(font);

    // Main game loop
    while (window.isOpen())
    {
        // Calculate delta time (time since last frame)
        float deltaTime = deltaClock.restart().asSeconds();
        gameTime += deltaTime;

        // FPS calculation
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f)
        {
            fps = frameCount / fpsClock.restart().asSeconds();
            frameCount = 0;
        }

        // Event handling (SFML 3.0 uses optional for pollEvent)
        while (std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            // ESC key to close (SFML 3.0 changed keyboard handling)
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }

        // ======= GAME LOGIC UPDATE =======
        // Update resources based on production rates and delta time
        food += foodPerSecond * deltaTime;

        // ======= RENDERING =======
        window.clear(sf::Color(20, 20, 30)); // Dark blue background

        // Display game info (without font for now - just using console)
        if (static_cast<int>(gameTime) % 2 == 0) // Print to console every ~2 seconds
        {
            static int lastPrint = 0;
            if (static_cast<int>(gameTime) != lastPrint)
            {
                lastPrint = static_cast<int>(gameTime);
                std::cout << "FPS: " << std::fixed << std::setprecision(1) << fps
                    << " | Food: " << std::setprecision(2) << food
                    << " | Time: " << std::setprecision(1) << gameTime << "s" << std::endl;
            }
        }

        window.display();
    }

    return 0;
}