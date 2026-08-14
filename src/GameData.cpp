#include "GameData.h"

#include <algorithm>
#include <random>
#include <utility>

namespace {

// Round-1 values. (Round 2 doubles these; see GameState::StartRound2.)
constexpr int kValues[8] = {100, 200, 300, 400, 500, 600, 700, 800};

struct ClueData {
    const wchar_t* text;
    const wchar_t* answer;
};

Category MakeCategory(const wchar_t* name, const ClueData (&clues)[8]) {
    Category category;
    category.name = name;
    for (int row = 0; row < 8; ++row) {
        Question& q = category.questions[row];
        q.value = kValues[row];
        q.text = clues[row].text;
        q.answer = clues[row].answer;
        q.used = false;
    }
    return category;
}

} // namespace

Board LoadHardcodedBoard() {
    static const ClueData science[8] = {
        {L"The gas humans need to breathe to survive", L"Oxygen"},
        {L"The powerhouse of the cell", L"Mitochondria"},
        {L"The chemical symbol for gold", L"Au"},
        {L"The force that pulls objects toward Earth", L"Gravity"},
        {L"The closest planet to the sun", L"Mercury"},
        {L"The scientist who developed the theory of relativity", L"Einstein"},
        {L"The process by which plants convert sunlight into energy", L"Photosynthesis"},
        {L"The subatomic particle with a negative charge", L"Electron"},
    };

    static const ClueData history[8] = {
        {L"The first President of the United States", L"Washington"},
        {L"The war fought between the North and South, 1861-1865", L"Civil War"},
        {L"The ancient Egyptian writing system of pictures and symbols", L"Hieroglyphics"},
        {L"The year World War II ended", L"1945"},
        {L"The wall that divided Berlin during the Cold War", L"Berlin Wall"},
        {L"The ship that sank in the North Atlantic in 1912", L"Titanic"},
        {L"The empire ruled by Julius Caesar", L"Roman Empire"},
        {L"The 1215 document that limited the power of English kings", L"Magna Carta"},
    };

    static const ClueData geography[8] = {
        {L"The largest continent by area", L"Asia"},
        {L"The longest river in the world", L"Nile"},
        {L"The country shaped like a boot", L"Italy"},
        {L"The capital of France", L"Paris"},
        {L"The largest ocean on Earth", L"Pacific"},
        {L"The smallest country in the world", L"Vatican City"},
        {L"The mountain range that separates Europe and Asia", L"Ural Mountains"},
        {L"The desert that covers most of northern Africa", L"Sahara"},
    };

    static const ClueData movies[8] = {
        {L"The 1997 film about a ship that sinks, starring Leonardo DiCaprio", L"Titanic"},
        {L"The wizard school in the Harry Potter series", L"Hogwarts"},
        {L"The director of Jaws and E.T.", L"Spielberg"},
        {L"The green ogre in a DreamWorks animated franchise", L"Shrek"},
        {L"The 1994 film where Tom Hanks runs across America", L"Forrest Gump"},
        {L"The fictional African nation home to Black Panther", L"Wakanda"},
        {L"The film trilogy based on Tolkien's Middle-earth", L"Lord of the Rings"},
        {L"The AI antagonist aboard the ship in 2001: A Space Odyssey", L"HAL"},
    };

    static const ClueData music[8] = {
        {L"The number of strings on a standard guitar", L"Six"},
        {L"The deaf composer of the Ninth Symphony", L"Beethoven"},
        {L"The singer known as the King of Rock and Roll", L"Elvis"},
        {L"The British band known as the Fab Four", L"Beatles"},
        {L"The pop star known as the Queen of Pop", L"Madonna"},
        {L"The Italian musical term meaning 'very loud'", L"Fortissimo"},
        {L"The instrument with 88 keys", L"Piano"},
        {L"The Baroque composer of 'The Four Seasons'", L"Vivaldi"},
    };

    static const ClueData sports[8] = {
        {L"The number of players on a basketball team on the court at once", L"Five"},
        {L"The trophy awarded to the NHL champion", L"Stanley Cup"},
        {L"The country that has won the most FIFA World Cups", L"Brazil"},
        {L"The sport played at Wimbledon", L"Tennis"},
        {L"The number of holes in a standard round of golf", L"Eighteen"},
        {L"The international athletic event held every four years", L"Olympics"},
        {L"The boxer known as 'The Greatest'", L"Muhammad Ali"},
        {L"The ancient Greek city that lends its name to a long-distance race", L"Marathon"},
    };

    static const ClueData literature[8] = {
        {L"The author of Romeo and Juliet", L"Shakespeare"},
        {L"The young wizard in J.K. Rowling's book series", L"Harry Potter"},
        {L"The whale in Herman Melville's novel", L"Moby Dick"},
        {L"The dystopian George Orwell novel featuring Big Brother", L"1984"},
        {L"The author of Pride and Prejudice", L"Jane Austen"},
        {L"The hobbit who carries the One Ring in Tolkien's trilogy", L"Frodo"},
        {L"The Russian author of War and Peace", L"Tolstoy"},
        {L"The Greek epic poem about the fall of Troy", L"The Iliad"},
    };

    static const ClueData foodAndDrink[8] = {
        {L"The main ingredient in guacamole", L"Avocado"},
        {L"The Italian dish of dough topped with cheese and tomato sauce", L"Pizza"},
        {L"The bean used to make chocolate", L"Cocoa"},
        {L"The French pastry shaped like a crescent", L"Croissant"},
        {L"The Mexican spirit distilled from the agave plant", L"Tequila"},
        {L"The Japanese dish of vinegared rice, often paired with raw fish", L"Sushi"},
        {L"The spice from the Crocus flower, the most expensive by weight", L"Saffron"},
        {L"The country of origin for Champagne", L"France"},
    };

    Board board;
    board.categories[0] = MakeCategory(L"Science", science);
    board.categories[1] = MakeCategory(L"History", history);
    board.categories[2] = MakeCategory(L"Geography", geography);
    board.categories[3] = MakeCategory(L"Movies", movies);
    board.categories[4] = MakeCategory(L"Music", music);
    board.categories[5] = MakeCategory(L"Sports", sports);
    board.categories[6] = MakeCategory(L"Literature", literature);
    board.categories[7] = MakeCategory(L"Food & Drink", foodAndDrink);
    board.difficulty = kStandardDictionaryDifficulty;
    return board;
}

void AssignRandomDailyDoubles(Board& board) {
    // Flatten the 8x8 grid into 64 (col, row) slots, clearing any stale
    // flag from a previous round along the way.
    std::array<std::pair<int, int>, 64> slots;
    int index = 0;
    for (int col = 0; col < 8; ++col) {
        for (int row = 0; row < 8; ++row) {
            board.categories[col].questions[row].isDailyDouble = false;
            slots[index++] = {col, row};
        }
    }

    // A fresh, properly-seeded engine each call keeps this simple; Daily
    // Double placement doesn't need to be reproducible or high-throughput,
    // just unpredictable to the player.
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(slots.begin(), slots.end(), rng);

    for (int i = 0; i < 2; ++i) {
        auto [col, row] = slots[i];
        board.categories[col].questions[row].isDailyDouble = true;
    }
}
