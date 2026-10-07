/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#ifndef MediaInfo_CountPluralH
#define MediaInfo_CountPluralH

#include <string>
#include <vector>

namespace MediaInfoLib
{
namespace CountPlural
{

enum Category { Zero, One, Two, Few, Many, Other };

struct Result
{
    bool Valid;
    bool KnownLocale;
    Category Value;
};

inline const char* Name(Category value)
{
    static const char* names[] = { "zero", "one", "two", "few", "many", "other" };
    return value >= Zero && value <= Other ? names[value] : names[Other];
}

namespace Detail
{

// Keep digits as text: neither large values nor visible decimal zeros may be lost.
struct Number
{
    std::string Integer;
    std::string Fraction;
    bool Whole;

    bool Parse(const std::string& text)
    {
        size_t start = !text.empty() && (text[0] == '-' || text[0] == '+') ? 1 : 0;
        size_t end = start;
        while (end < text.size() && text[end] >= '0' && text[end] <= '9')
            ++end;
        if (end == start)
            return false;
        while (start + 1 < end && text[start] == '0')
            ++start;
        Integer.assign(text, start, end - start);
        Fraction.clear();
        if (end < text.size())
        {
            if (text[end++] != '.')
                return false;
            start = end;
            while (end < text.size() && text[end] >= '0' && text[end] <= '9')
                ++end;
            if (end == start || end != text.size())
                return false;
            Fraction.assign(text, start, end - start);
        }
        Whole = Fraction.find_first_not_of('0') == std::string::npos;
        return true;
    }

    bool Decimal() const { return !Fraction.empty(); }

    // Only small exact values and digit endings are needed by the supported languages.
    unsigned Small() const { return Integer.size() == 1 ? Integer[0] - '0' : 10; }

    static unsigned Ending(const std::string& digits)
    {
        if (digits.empty())
            return 0;
        unsigned value = digits[digits.size() - 1] - '0';
        if (digits.size() > 1)
            value += 10 * (digits[digits.size() - 2] - '0');
        return value;
    }

    bool Millions() const
    {
        return !Decimal() && Integer.size() > 6 && Integer.compare(Integer.size() - 6, 6, "000000") == 0;
    }
};

enum Rule
{
    Unknown, Invariant, IntegerOne, NumericOne, IntegerOneMillions,
    NumericOneMillions, SmallIntegerPart, SmallIntegerPartMillions,
    Persian, Danish, CzechSlovak, Polish, EastSlavic, Belarusian,
    Croatian, Lithuanian, Romanian, Arabic
};

// These rules cover the languages shipped in MediaInfo's catalogs.
inline Rule LocaleRule(std::string locale)
{
    if (locale.empty())
        return Unknown;
    for (size_t pos = 0; pos < locale.size(); ++pos)
    {
        char& c = locale[pos];
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        else if (c == '_')
            c = '-';
        if (c == '-')
        {
            if (!pos || pos + 1 == locale.size() || locale[pos - 1] == '-')
                return Unknown;
        }
        else if ((c < 'a' || c > 'z') && (c < '0' || c > '9'))
            return Unknown;
    }
    const std::string language = locale.substr(0, locale.find('-'));
    if (language == "id" || language == "ja" || language == "ko" || language == "th" || language == "zh")
        return Invariant;
    if (language == "de" || language == "en" || language == "gl" || language == "nl" || language == "sv")
        return IntegerOne;
    if (language == "bg" || language == "el" || language == "eu" || language == "hu" ||
        language == "ka" || language == "sq" || language == "tr")
        return NumericOne;
    if (language == "ca" || language == "it")
        return IntegerOneMillions;
    if (language == "es")
        return NumericOneMillions;
    if (language == "hy")
        return SmallIntegerPart;
    if (language == "fr")
        return SmallIntegerPartMillions;
    if (language == "pt")
    {
        // MediaInfo's default Portuguese catalog is European; Brazil is explicit.
        size_t separator = locale.find('-', 3);
        std::string region = locale.size() > 3 ? locale.substr(3, separator - 3) : std::string();
        if (region.size() == 4 && separator != std::string::npos) // Optional script subtag.
        {
            size_t start = separator + 1;
            region = locale.substr(start, locale.find('-', start) - start);
        }
        return region == "br" ? SmallIntegerPartMillions : IntegerOneMillions;
    }
    if (language == "fa") return Persian;
    if (language == "da") return Danish;
    if (language == "cs" || language == "sk") return CzechSlovak;
    if (language == "pl") return Polish;
    if (language == "ru" || language == "uk") return EastSlavic;
    if (language == "be") return Belarusian;
    if (language == "hr") return Croatian;
    if (language == "lt") return Lithuanian;
    if (language == "ro") return Romanian;
    if (language == "ar") return Arabic;
    return Unknown;
}

inline Category SlavicEnding(unsigned ending)
{
    if (ending >= 11 && ending <= 14)
        return Many;
    switch (ending % 10)
    {
        case 1: return One;
        case 2:
        case 3:
        case 4: return Few;
        default: return Many;
    }
}

inline Category SelectRule(Rule rule, const Number& number)
{
    const unsigned small = number.Small();
    const unsigned ending = Number::Ending(number.Integer);
    switch (rule)
    {
        case IntegerOneMillions:
        case NumericOneMillions:
        case SmallIntegerPartMillions:
            if (number.Millions())
                return Many;
            if (rule == IntegerOneMillions) rule = IntegerOne;
            else if (rule == NumericOneMillions) rule = NumericOne;
            else rule = SmallIntegerPart;
            break;
        default: break;
    }
    switch (rule)
    {
        case IntegerOne: return !number.Decimal() && small == 1 ? One : Other;
        case NumericOne: return number.Whole && small == 1 ? One : Other;
        case SmallIntegerPart: return small <= 1 ? One : Other;
        case Persian: return small == 0 || (number.Whole && small == 1) ? One : Other;
        case Danish: return (number.Whole && small == 1) || (!number.Whole && small <= 1) ? One : Other;
        case CzechSlovak:
            if (number.Decimal()) return Many;
            if (small == 1) return One;
            return small >= 2 && small <= 4 ? Few : Other;
        case Polish:
            if (number.Decimal()) return Other;
            if (small == 1) return One;
            return SlavicEnding(ending) == Few ? Few : Many;
        case EastSlavic:
            return number.Decimal() ? Other : SlavicEnding(ending);
        case Belarusian:
            return number.Whole ? SlavicEnding(ending) : Other;
        case Croatian:
        {
            // Decimal forms follow the visible fractional digits, including zeros.
            Category form = SlavicEnding(number.Decimal() ? Number::Ending(number.Fraction) : ending);
            return form == Many ? Other : form;
        }
        case Lithuanian:
            if (!number.Whole) return Many;
            if (ending >= 11 && ending <= 19) return Other;
            if (ending % 10 == 1) return One;
            return ending % 10 >= 2 ? Few : Other;
        case Romanian:
            if (!number.Decimal() && small == 1) return One;
            return number.Decimal() || small == 0 || (ending >= 1 && ending <= 19) ? Few : Other;
        case Arabic:
            if (!number.Whole) return Other;
            if (small == 0) return Zero;
            if (small == 1) return One;
            if (small == 2) return Two;
            if (ending >= 3 && ending <= 10) return Few;
            return ending >= 11 ? Many : Other;
        default: return Other;
    }
}

} // namespace Detail

// Catalog expressions are compiled once: at most 1024 bytes, 16 nested parentheses,
// and constants up to 1000000. Count operands retain exact digits and decimal zeros.
class Rules
{
    struct Instruction
    {
        enum Operation { False, True, Compare, And, Or } Op;
        enum Comparison { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual } Test;
        char Operand;
        unsigned Modulus;
        unsigned Value;

        Instruction(Operation op) : Op(op), Test(Equal), Operand('i'), Modulus(0), Value(0) {}
    };
    typedef std::vector<Instruction> Program;
    Program Programs[Other];

    class Parser
    {
        const std::string& Text;
        Program& Output;
        size_t Position;

        void Space()
        {
            while (Position < Text.size() && (Text[Position] == ' ' || Text[Position] == '\t' ||
                Text[Position] == '\r' || Text[Position] == '\n'))
                ++Position;
        }

        bool Take(const char* token)
        {
            Space();
            size_t length = std::char_traits<char>::length(token);
            if (Text.compare(Position, length, token) != 0)
                return false;
            Position += length;
            return true;
        }

        bool Constant(unsigned& value)
        {
            Space();
            size_t start = Position;
            value = 0;
            while (Position < Text.size() && Text[Position] >= '0' && Text[Position] <= '9')
            {
                unsigned digit = Text[Position++] - '0';
                if (value > (1000000 - digit) / 10)
                    return false;
                value = value * 10 + digit;
            }
            return Position != start;
        }

        bool Term(unsigned depth)
        {
            if (Take("("))
                return depth < 16 && Disjunction(depth + 1) && Take(")");
            if (Take("true"))
            {
                Output.push_back(Instruction(Instruction::True));
                return true;
            }
            if (Take("false"))
            {
                Output.push_back(Instruction(Instruction::False));
                return true;
            }
            if (Position == Text.size())
                return false;
            Instruction instruction(Instruction::Compare);
            instruction.Operand = Text[Position++];
            if (instruction.Operand != 'i' && instruction.Operand != 'v' && instruction.Operand != 'f')
                return false;
            if (Take("%") && (!Constant(instruction.Modulus) || !instruction.Modulus))
                return false;
            if (Take("==")) instruction.Test = Instruction::Equal;
            else if (Take("!=")) instruction.Test = Instruction::NotEqual;
            else if (Take("<=")) instruction.Test = Instruction::LessEqual;
            else if (Take(">=")) instruction.Test = Instruction::GreaterEqual;
            else if (Take("<")) instruction.Test = Instruction::Less;
            else if (Take(">")) instruction.Test = Instruction::Greater;
            else return false;
            if (!Constant(instruction.Value))
                return false;
            Output.push_back(instruction);
            return true;
        }

        bool Conjunction(unsigned depth)
        {
            if (!Term(depth))
                return false;
            while (Take("&&"))
            {
                if (!Term(depth))
                    return false;
                Output.push_back(Instruction(Instruction::And));
            }
            return true;
        }

        bool Disjunction(unsigned depth)
        {
            if (!Conjunction(depth))
                return false;
            while (Take("||"))
            {
                if (!Conjunction(depth))
                    return false;
                Output.push_back(Instruction(Instruction::Or));
            }
            return true;
        }

    public:
        Parser(const std::string& text, Program& output) : Text(text), Output(output), Position(0) {}

        bool Parse()
        {
            if (Text.size() > 1024 || !Disjunction(0))
                return false;
            Space();
            return Position == Text.size();
        }
    };

    static int CompareDigits(const std::string& digits, unsigned modulus, unsigned value)
    {
        if (modulus)
        {
            unsigned remainder = 0;
            for (size_t pos = 0; pos < digits.size(); ++pos)
                remainder = (remainder * 10 + digits[pos] - '0') % modulus;
            return remainder < value ? -1 : remainder > value ? 1 : 0;
        }
        size_t start = digits.find_first_not_of('0');
        if (start == std::string::npos)
            return value ? -1 : 0;
        unsigned width = 1;
        for (unsigned remaining = value; remaining >= 10; remaining /= 10)
            ++width;
        size_t length = digits.size() - start;
        if (length != width)
            return length < width ? -1 : 1;
        unsigned number = 0;
        for (size_t pos = start; pos < digits.size(); ++pos)
            number = number * 10 + digits[pos] - '0';
        return number < value ? -1 : number > value ? 1 : 0;
    }

    static bool Matches(const Program& program, const Detail::Number& number)
    {
        std::vector<bool> stack;
        stack.reserve(program.size());
        for (size_t pos = 0; pos < program.size(); ++pos)
        {
            const Instruction& instruction = program[pos];
            if (instruction.Op == Instruction::And || instruction.Op == Instruction::Or)
            {
                bool right = stack.back();
                stack.pop_back();
                bool left = stack.back();
                stack.back() = instruction.Op == Instruction::And ? left && right : left || right;
            }
            else if (instruction.Op == Instruction::Compare)
            {
                int comparison;
                if (instruction.Operand == 'v')
                {
                    size_t length = number.Fraction.size();
                    if (instruction.Modulus)
                        length %= instruction.Modulus;
                    comparison = length < instruction.Value ? -1 : length > instruction.Value ? 1 : 0;
                }
                else
                    comparison = CompareDigits(instruction.Operand == 'i' ? number.Integer : number.Fraction,
                        instruction.Modulus, instruction.Value);
                bool match = false;
                switch (instruction.Test)
                {
                    case Instruction::Equal: match = comparison == 0; break;
                    case Instruction::NotEqual: match = comparison != 0; break;
                    case Instruction::Less: match = comparison < 0; break;
                    case Instruction::LessEqual: match = comparison <= 0; break;
                    case Instruction::Greater: match = comparison > 0; break;
                    case Instruction::GreaterEqual: match = comparison >= 0; break;
                }
                stack.push_back(match);
            }
            else
                stack.push_back(instruction.Op == Instruction::True);
        }
        return !stack.empty() && stack.back();
    }

public:
    // Empty expressions disable a category. Invalid replacements also clear it.
    // "other" is implicit and cannot have a predicate.
    bool Set(Category category, const std::string& expression)
    {
        if (category < Zero || category >= Other)
            return false;
        Program& program = Programs[category];
        program.clear();
        if (expression.empty())
            return true;
        Parser parser(expression, program);
        if (parser.Parse())
            return true;
        program.clear();
        return false;
    }

    Result Select(const std::string& count) const
    {
        Detail::Number number;
        Result result;
        result.Valid = number.Parse(count);
        result.KnownLocale = true;
        result.Value = Other;
        if (result.Valid)
            for (int category = Zero; category < Other; ++category)
                if (Matches(Programs[category], number))
                {
                    result.Value = static_cast<Category>(category);
                    break;
                }
        return result;
    }
};

inline bool KnownLocale(const std::string& locale)
{
    return Detail::LocaleRule(locale) != Detail::Unknown;
}

// Count: optional ASCII sign, digits, optional '.' followed by digits.
// Signs do not affect cardinal categories. Expressions and exponents are invalid.
inline Result Select(const std::string& locale, const std::string& count)
{
    const Detail::Rule rule = Detail::LocaleRule(locale);
    Detail::Number number;
    Result result;
    result.Valid = number.Parse(count);
    result.KnownLocale = rule != Detail::Unknown;
    result.Value = result.Valid && result.KnownLocale ? Detail::SelectRule(rule, number) : Other;
    return result;
}

} // namespace CountPlural
} // namespace MediaInfoLib

#endif
