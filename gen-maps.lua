local function build_vtg()
    -- VT100 Graphics
    local vtg_kv = {
        {0x2B, 0x2192}, {0x2C, 0x2190}, {0x2D, 0x2191}, {0x2E, 0x2193},
        {0x30, 0x25AE}, {0x49, 0x2603}, {0x60, 0x25C6}, {0x61, 0x2592},
        {0x66, 0x00B0}, {0x67, 0x00B1}, {0x68, 0x2591}, {0x6A, 0x2518},
        {0x6B, 0x2510}, {0x6C, 0x250C}, {0x6D, 0x2514}, {0x6E, 0x253C},
        {0x6F, 0x23BA}, {0x70, 0x23BB}, {0x71, 0x2500}, {0x72, 0x23BC},
        {0x73, 0x23BD}, {0x74, 0x251C}, {0x75, 0x2524}, {0x76, 0x2534},
        {0x77, 0x252C}, {0x78, 0x2502}, {0x79, 0x2264}, {0x7A, 0x2265},
        {0x7B, 0x03C0}, {0x7C, 0x2260}, {0x7D, 0x00A3}, {0x7E, 0x00B7},
    }
    local vtg = {}
    for i, kv in ipairs(vtg_kv) do
        local k, v = unpack(kv)
        vtg[k] = v
    end
    return vtg
end

local function build_ibm()
    -- Code Page 437
    local ibm_kv = {
        {0x01, 0x263A}, {0x02, 0x263B}, {0x03, 0x2665}, {0x04, 0x2666},
        {0x05, 0x2663}, {0x06, 0x2660}, {0x07, 0x2022}, {0x08, 0x25D8},
        {0x09, 0x2218}, {0x0A, 0x25D9}, {0x0B, 0x2642}, {0x0C, 0x2640},
        {0x0D, 0x266A}, {0x0E, 0x266B}, {0x0F, 0x263C}, {0x10, 0x25BA},
        {0x11, 0x25C4}, {0x12, 0x2195}, {0x13, 0x203C}, {0x14, 0x00B6},
        {0x15, 0x00A7}, {0x16, 0x25AC}, {0x17, 0x21A8}, {0x18, 0x2191},
        {0x19, 0x2193}, {0x1A, 0x2192}, {0x1B, 0x2190}, {0x1C, 0x221F},
        {0x1D, 0x2194}, {0x1E, 0x25B2}, {0x1F, 0x25BC}, {0x7F, 0x2302},

        {0x80, 0x00C7}, {0x81, 0x00FC}, {0x82, 0x00E9}, {0x83, 0x00E2},
        {0x84, 0x00E4}, {0x85, 0x00E0}, {0x86, 0x00E5}, {0x87, 0x00E7},
        {0x88, 0x00EA}, {0x89, 0x00EB}, {0x8A, 0x00E8}, {0x8B, 0x00EF},
        {0x8C, 0x00EE}, {0x8D, 0x00EC}, {0x8E, 0x00C4}, {0x8F, 0x00C5},
        {0x90, 0x00C9}, {0x91, 0x00E6}, {0x92, 0x00C6}, {0x93, 0x00F4},
        {0x94, 0x00F6}, {0x95, 0x00F2}, {0x96, 0x00FB}, {0x97, 0x00F9},
        {0x98, 0x00FF}, {0x99, 0x00D6}, {0x9A, 0x00DC}, {0x9B, 0x00A2},
        {0x9C, 0x00A3}, {0x9D, 0x00A5}, {0x9E, 0x20A7}, {0x9F, 0x0192},
        {0xA0, 0x00E1}, {0xA1, 0x00ED}, {0xA2, 0x00F3}, {0xA3, 0x00FA},
        {0xA4, 0x00F1}, {0xA5, 0x00D1}, {0xA6, 0x00AA}, {0xA7, 0x00BA},
        {0xA8, 0x00BF}, {0xA9, 0x2310}, {0xAA, 0x00AC}, {0xAB, 0x00BD},
        {0xAC, 0x00BC}, {0xAD, 0x00A1}, {0xAE, 0x00AB}, {0xAF, 0x00BB},
        {0xB0, 0x2591}, {0xB1, 0x2592}, {0xB2, 0x2593}, {0xB3, 0x2502},
        {0xB4, 0x2524}, {0xB5, 0x2561}, {0xB6, 0x2562}, {0xB7, 0x2556},
        {0xB8, 0x2555}, {0xB9, 0x2563}, {0xBA, 0x2551}, {0xBB, 0x2557},
        {0xBC, 0x255D}, {0xBD, 0x255C}, {0xBE, 0x255B}, {0xBF, 0x2510},
        {0xC0, 0x2514}, {0xC1, 0x2534}, {0xC2, 0x252C}, {0xC3, 0x251C},
        {0xC4, 0x2500}, {0xC5, 0x253C}, {0xC6, 0x255E}, {0xC7, 0x255F},
        {0xC8, 0x255A}, {0xC9, 0x2554}, {0xCA, 0x2569}, {0xCB, 0x2566},
        {0xCC, 0x2560}, {0xCD, 0x2550}, {0xCE, 0x256C}, {0xCF, 0x2567},
        {0xD0, 0x2568}, {0xD1, 0x2564}, {0xD2, 0x2565}, {0xD3, 0x2559},
        {0xD4, 0x2558}, {0xD5, 0x2552}, {0xD6, 0x2553}, {0xD7, 0x256B},
        {0xD8, 0x256A}, {0xD9, 0x2518}, {0xDA, 0x250C}, {0xDB, 0x2588},
        {0xDC, 0x2584}, {0xDD, 0x258C}, {0xDE, 0x2590}, {0xDF, 0x2580},
        {0xE0, 0x03B1}, {0xE1, 0x00DF}, {0xE2, 0x0393}, {0xE3, 0x03C0},
        {0xE4, 0x03A3}, {0xE5, 0x03C3}, {0xE6, 0x00B5}, {0xE7, 0x03C4},
        {0xE8, 0x03A6}, {0xE9, 0x0398}, {0xEA, 0x03A9}, {0xEB, 0x03B4},
        {0xEC, 0x221E}, {0xED, 0x03C6}, {0xEE, 0x03B5}, {0xEF, 0x2229},
        {0xF0, 0x2261}, {0xF1, 0x00B1}, {0xF2, 0x2265}, {0xF3, 0x2264},
        {0xF4, 0x2320}, {0xF5, 0x2321}, {0xF6, 0x00F7}, {0xF7, 0x2248},
        {0xF8, 0x00B0}, {0xF9, 0x2219}, {0xFA, 0x00B7}, {0xFB, 0x221A},
        {0xFC, 0x207F}, {0xFD, 0x00B2}, {0xFE, 0x25A0}, {0xFF, 0x00A0}
    }
    local ibm = {}
    for i, kv in ipairs(ibm_kv) do
        local k, v = unpack(kv)
        ibm[k] = v
    end
    return ibm
end

local vtg = build_vtg()
local ibm = build_ibm()

local function gen_header(name, tab)
    local fname = name..".h"
    local fp = io.open(fname, "w")
    fp:write("static uint16_t "..name.."[0x100] = {")
    local i = 0
    for j = 1, 32 do
        fp:write("\n   ")
        for k = 1, 8 do
            local c = tab[i] or i
            fp:write((" 0x%04X,"):format(c))
            i = i + 1
        end
    end
    fp:write("\n};\n")
    fp:close()
end

gen_header("cs_vtg", vtg)
gen_header("cs_437", ibm)