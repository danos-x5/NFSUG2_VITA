#ifndef UG2_FEPACKAGE_H_
#define UG2_FEPACKAGE_H_

// FEng package bytes (FRONTB.LZC): unpack, parse, edit objects, rename and write back. No game access; host tested.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ug2 { namespace fepackage {

const uint32_t ChunkPackage = 0x30210;     // FRONTB chunk: name hash, then a packed package
const uint32_t FlagHidden = 0x1, FlagButton = 0x10000000;

// Upper-cased 33-multiply hash of package and object names.
uint32_t nameHash(const std::string& name);
// Unpacks a JDLZ or RAWW block; false otherwise.
bool unpack(const uint8_t* data, size_t size, std::vector<uint8_t>* out);
// FRONTB chunk 0x30210: name hash, then a RAWW block.
std::vector<uint8_t> packageChunk(const std::string& name, const std::vector<uint8_t>& package);

struct Tag
{
    uint16_t id;        // two characters, "Ot" is 0x744f
    std::vector<uint8_t> data;
};

struct Chunk
{
    uint32_t id;
    std::vector<uint8_t> data;
};

struct Object
{
    std::vector<Chunk> chunks;      // ObjD (data left empty, see tags), scripts, message responses
    std::vector<Tag> tags;          // the ObjD chunk's tags
};

class Package
{
public:
    bool parse(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> write() const;

    std::string name() const { return m_name; }
    void rename(const std::string& name);

    size_t objectCount() const { return m_objects.size(); }
    // Objects with this name hash, in list order.
    std::vector<size_t> find(uint32_t hash) const;
    uint32_t type(size_t object) const;
    uint32_t hash(size_t object) const;
    uint32_t guid(size_t object) const;
    uint32_t parent(size_t object) const;      // 0 at the top level
    uint32_t flags(size_t object) const;
    void setFlags(size_t object, uint32_t flags);
    bool hasTag(size_t object, uint16_t id) const;
    uint32_t tag32(size_t object, uint16_t id) const;
    bool setTag32(size_t object, uint16_t id, uint32_t value);
    // The object and every object below it.
    std::vector<size_t> subtree(size_t object) const;
    // Hides these objects and their children; how many changed.
    size_t hide(uint32_t hash);
    // Copies an object subtree after itself; the copy's index.
    size_t clone(size_t object, uint32_t newHash);
    uint32_t topLevelCount() const { return m_topLevel; }
    uint32_t buttonCount() const;

private:
    struct Entry
    {
        bool object;
        Chunk chunk;        // when not an object
        size_t index;       // into m_objects when an object
    };
    const Tag* findTag(size_t object, uint16_t id) const;
    Tag* findTag(size_t object, uint16_t id);
    std::vector<uint8_t> header() const;

    std::vector<Chunk> m_top;           // top-level chunks; the Obj chunk and PkHd are rebuilt
    size_t m_objChunk = 0, m_headerChunk = 0;
    std::vector<uint8_t> m_rawHeader;   // the PkHd body as read
    bool m_renamed = false;
    std::string m_name, m_path;
    uint32_t m_version = 0, m_zero = 0, m_resources = 0, m_topLevel = 0;
    std::vector<Entry> m_entries;       // the Obj chunk's children in order
    std::vector<Object> m_objects;
};

}}

#endif /* !UG2_FEPACKAGE_H_ */
