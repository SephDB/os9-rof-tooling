#ifndef OS9_DETAIL_EA_H
#define OS9_DETAIL_EA_H

namespace {
    template<typename T, T def> struct opt
    {
        T value = def;
        constexpr opt(T v) : value(v) {}
        constexpr bool empty() const { return value == def; }
    };
}// namespace

enum class InstructionSize { Byte, Word, Long };

inline char to_char(InstructionSize s)
{
    switch (s) {
    case InstructionSize::Byte:
        return 'B';
    case InstructionSize::Word:
        return 'W';
    case InstructionSize::Long:
        return 'L';
    }
}

template<uint8_t Loc> struct BitWL
{
    static constexpr InstructionSize size(uint16_t n)
    {
        return ((n & (1 << Loc)) == 0) ? InstructionSize::Word : InstructionSize::Long;
    }
};

namespace ea {
    namespace ExtensionHelpers {
        template<int N> constexpr auto const_num = [](InstructionSize) { return N; };

        constexpr auto eq_ins_size = [](InstructionSize s) {
            switch (s) {
            case InstructionSize::Long:
                return 2;
            default:
                return 1;
            }
        };
    }// namespace ExtensionHelpers

    template<uint8_t Mode, opt<uint8_t, 0xff> Reg = 0xff, auto Extension = ExtensionHelpers::const_num<0>> struct EA
    {
        constexpr static uint8_t mode = Mode;
        constexpr static std::optional<uint8_t> fixed_reg = Reg.empty() ? std::nullopt : std::optional{ Reg.value };
        constexpr static auto num_extensions = Extension;
    };

    struct DataReg : EA<0>
    {
        uint8_t reg_num;
        constexpr explicit DataReg(uint8_t r) : reg_num(r) {}
        std::string to_string() const;
    };

    struct AddressReg : EA<1>
    {
        uint8_t reg_num;
        constexpr explicit AddressReg(uint8_t r) : reg_num(r) {}
        std::string to_string() const;
    };

    struct IndirAddress : EA<0b10> {
        uint8_t reg_num;
        constexpr explicit IndirAddress(uint8_t r) : reg_num(r) {}
        std::string to_string() const;
    };

    struct IndirAddressInc : EA<0b11> {
        uint8_t reg_num;
        constexpr explicit IndirAddressInc(uint8_t r) : reg_num(r) {}
        std::string to_string() const;
    };

    struct IndirAddressDec : EA<0b100> {
        uint8_t reg_num;
        constexpr explicit IndirAddressDec(uint8_t r) : reg_num(r) {}
        std::string to_string() const;
    };

    template<typename EA_t> constexpr EA_t parse_ea(uint8_t reg)
    {
        if constexpr (!EA_t::fixed_reg) { return EA_t{ reg }; }
        else {
            return EA_t{};
        }
    }

    template<typename T> using id = std::type_identity<T>;

    template<typename... EAs> struct EAList
    {
        static std::string parse(uint8_t mode, uint8_t reg)
        {
            std::string res;
            auto test = [&]<typename I>(id<I>) -> bool {
                if (I::mode == mode && (!I::fixed_reg || *I::fixed_reg == reg)) {
                    res = parse_ea<I>(reg).to_string();
                    return true;
                }
                return false;
            };
            (test(id<EAs>{}) || ...);
            return res;
        }
    };

    using AllEAs = EAList<DataReg, AddressReg, IndirAddress, IndirAddressInc, IndirAddressDec>;
    using AddressOnly = EAList<AddressReg, IndirAddress, IndirAddressInc, IndirAddressDec>;


    template<uint8_t Val> struct Fixed
    {
        constexpr static uint8_t value(uint16_t) { return Val; }
    };

    template<uint8_t Loc, uint8_t Length = 3> struct Read
    {
        constexpr static uint8_t value(uint16_t val) { return (val >> Loc) & ((1 << Length) - 1); }
    };

    template<typename Mode, typename Reg, typename AllowedEAs> struct EAReader
    {
        static std::string parse(uint16_t val) { return AllowedEAs::parse(Mode::value(val), Reg::value(val)); }
    };

    template<typename Allowed>
    using DefaultEAReader = EAReader<Read<3, 3>, Read<0, 3>, Allowed>;

}// namespace ea


#endif
