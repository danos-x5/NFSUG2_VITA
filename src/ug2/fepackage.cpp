// FEng package bytes: the chunk layout (FEn, PkHd, Obj with one FOb chunk per object, ObjD tags) and JDLZ unpacking.

#include "fepackage.h"

#include <algorithm>
#include <cstring>

namespace ug2 { namespace fepackage {

namespace {

const uint32_t IdPackage = 0xe76e4546;      // "FEn\xe7"
const uint32_t IdHeader = 0x64486b50;       // "PkHd"
const uint32_t IdObjects = 0xcc6a624f;      // "Obj\xcc"
const uint32_t IdObject = 0xea624f46;       // "FOb\xea"
const uint32_t IdObjectData = 0x446a624f;   // "ObjD"
const uint32_t IdButtons = 0x6e747542;      // "Butn"
const uint32_t Jdlz = 0x5a4c444a, Raww = 0x57574152;
const uint16_t TagType = 0x744f, TagHash = 0x684f, TagParams = 0x504f, TagParent = 0x4150;

uint32_t get32(const uint8_t* p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

void put32(std::vector<uint8_t>& out, uint32_t v)
{
    const uint8_t* b = reinterpret_cast<const uint8_t*>(&v);
    out.insert(out.end(), b, b + 4);
}

void set32(std::vector<uint8_t>& data, size_t at, uint32_t v)
{
    memcpy(&data[at], &v, 4);
}

bool readChunks(const uint8_t* p, size_t size, std::vector<Chunk>* out)
{
    size_t at = 0;
    while (at < size)
    {
        if (size - at < 8)
            return false;
        const uint32_t id = get32(p + at), length = get32(p + at + 4);
        if (length > size - at - 8)
            return false;
        out->push_back({ id, std::vector<uint8_t>(p + at + 8, p + at + 8 + length) });
        at += 8 + length;
    }
    return true;
}

void writeChunk(std::vector<uint8_t>& out, uint32_t id, const std::vector<uint8_t>& data)
{
    put32(out, id);
    put32(out, uint32_t(data.size()));
    out.insert(out.end(), data.begin(), data.end());
}

bool readTags(const std::vector<uint8_t>& data, std::vector<Tag>* out)
{
    size_t at = 0;
    while (at < data.size())
    {
        if (data.size() - at < 4)
            return false;
        uint16_t id, length;
        memcpy(&id, &data[at], 2);
        memcpy(&length, &data[at + 2], 2);
        if (length > data.size() - at - 4)
            return false;
        out->push_back({ id, std::vector<uint8_t>(data.begin() + long(at + 4), data.begin() + long(at + 4 + length)) });
        at += 4 + length;
    }
    return true;
}

std::vector<uint8_t> writeTags(const std::vector<Tag>& tags)
{
    std::vector<uint8_t> out;
    for (const Tag& t : tags)
    {
        const uint16_t length = uint16_t(t.data.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&t.id), reinterpret_cast<const uint8_t*>(&t.id) + 2);
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&length), reinterpret_cast<const uint8_t*>(&length) + 2);
        out.insert(out.end(), t.data.begin(), t.data.end());
    }
    return out;
}

// JDLZ: literal and match flag streams, two match forms.
bool unpackJdlz(const uint8_t* data, size_t size, std::vector<uint8_t>* out)
{
    if (size < 0x12 || data[4] != 2)
        return false;
    const uint32_t unpacked = get32(data + 8), packed = get32(data + 12);
    if (packed > size)
        return false;
    out->clear();
    out->reserve(unpacked);
    uint32_t flags1 = data[0x10] | 0x100, flags2 = data[0x11] | 0x100;
    size_t src = 0x12;
    int64_t left = int64_t(packed) - 0x12;
    while (left > 0)
    {
        if (flags1 & 1)
        {
            if (src + 2 > size)
                return false;
            const uint32_t b0 = data[src], b1 = data[src + 1];
            uint32_t length, distance;
            if (flags2 & 1)
            {
                length = (((b0 & 0xf0) << 4) | b1) + 3;
                distance = (b0 & 0xf) + 1;
            }
            else
            {
                distance = (((b0 & 0xe0) << 3) | b1) + 0x11;
                length = (b0 & 0x1f) + 3;
            }
            if (distance > out->size())
                return false;
            for (uint32_t i = 0; i < length; ++i)
                out->push_back((*out)[out->size() - distance]);
            flags2 >>= 1;
            src += 2;
            left -= 2;
        }
        else
        {
            if (out->size() < unpacked)
            {
                if (src >= size)
                    return false;
                out->push_back(data[src++]);
            }
            --left;
        }
        flags1 >>= 1;
        if (flags1 == 1 && left > 0)
        {
            flags1 = data[src++] | 0x100;
            --left;
        }
        if (flags2 == 1 && left > 0)
        {
            flags2 = data[src++] | 0x100;
            --left;
        }
    }
    if (out->size() < unpacked)
        return false;
    out->resize(unpacked);
    return true;
}

}

uint32_t nameHash(const std::string& name)
{
    uint32_t hash = 0xFFFFFFFF;
    for (char c : name)
        hash = uint32_t(uint8_t(c >= 'a' && c <= 'z' ? c - 32 : c)) + 33 * hash;
    return hash;
}

bool unpack(const uint8_t* data, size_t size, std::vector<uint8_t>* out)
{
    if (size < 0x10)
        return false;
    if (get32(data) == Jdlz)
        return unpackJdlz(data, size, out);
    if (get32(data) == Raww && data[4] == 1 && get32(data + 8) <= size - 0x10)
    {
        out->assign(data + 0x10, data + 0x10 + get32(data + 8));
        return true;
    }
    return false;
}

std::vector<uint8_t> packageChunk(const std::string& name, const std::vector<uint8_t>& package)
{
    std::vector<uint8_t> out;
    put32(out, ChunkPackage);
    put32(out, uint32_t(4 + 0x10 + package.size()));
    put32(out, nameHash(name));
    put32(out, Raww);
    put32(out, 1);
    put32(out, uint32_t(package.size()));
    put32(out, uint32_t(package.size() + 0x10));
    out.insert(out.end(), package.begin(), package.end());
    return out;
}

bool Package::parse(const std::vector<uint8_t>& bytes)
{
    *this = Package();
    if (bytes.size() < 8 || get32(bytes.data()) != IdPackage || get32(bytes.data() + 4) != bytes.size() - 8)
        return false;
    if (!readChunks(bytes.data() + 8, bytes.size() - 8, &m_top))
        return false;
    bool haveHeader = false, haveObjects = false;
    for (size_t i = 0; i < m_top.size(); ++i)
    {
        const Chunk& c = m_top[i];
        if (c.id == IdHeader && !haveHeader)
        {
            if (c.data.size() < 24)
                return false;
            const uint8_t* p = c.data.data();
            m_version = get32(p);
            m_zero = get32(p + 4);
            m_resources = get32(p + 8);
            m_topLevel = get32(p + 12);
            const uint32_t nameLength = get32(p + 16), pathLength = get32(p + 20);
            if (nameLength == 0 || pathLength == 0 || 24 + size_t(nameLength) + pathLength > c.data.size())
                return false;
            m_name.assign(reinterpret_cast<const char*>(p + 24), strnlen(reinterpret_cast<const char*>(p + 24), nameLength));
            m_path.assign(reinterpret_cast<const char*>(p + 24 + nameLength),
                          strnlen(reinterpret_cast<const char*>(p + 24 + nameLength), pathLength));
            m_rawHeader = c.data;
            m_headerChunk = i;
            haveHeader = true;
        }
        else if (c.id == IdObjects && !haveObjects)
        {
            std::vector<Chunk> children;
            if (!readChunks(c.data.data(), c.data.size(), &children))
                return false;
            for (Chunk& child : children)
            {
                if (child.id != IdObject)
                {
                    m_entries.push_back({ false, std::move(child), 0 });
                    continue;
                }
                Object object;
                if (!readChunks(child.data.data(), child.data.size(), &object.chunks))
                    return false;
                bool haveData = false;
                for (Chunk& sub : object.chunks)
                {
                    if (sub.id == IdObjectData && !haveData)
                    {
                        if (!readTags(sub.data, &object.tags))
                            return false;
                        sub.data.clear();
                        haveData = true;
                    }
                }
                if (!haveData)
                    return false;
                m_entries.push_back({ true, Chunk(), m_objects.size() });
                m_objects.push_back(std::move(object));
            }
            m_objChunk = i;
            haveObjects = true;
        }
    }
    return haveHeader && haveObjects;
}

std::vector<uint8_t> Package::header() const
{
    if (!m_renamed && get32(m_rawHeader.data() + 12) == m_topLevel)
        return m_rawHeader;
    std::vector<uint8_t> out;
    put32(out, m_version);
    put32(out, m_zero);
    put32(out, m_resources);
    put32(out, m_topLevel);
    put32(out, uint32_t(m_name.size() + 1));
    put32(out, uint32_t(m_path.size() + 1));
    out.insert(out.end(), m_name.c_str(), m_name.c_str() + m_name.size() + 1);
    out.insert(out.end(), m_path.c_str(), m_path.c_str() + m_path.size() + 1);
    out.resize((out.size() + 3) & ~size_t(3), 0);
    return out;
}

std::vector<uint8_t> Package::write() const
{
    std::vector<uint8_t> body;
    for (size_t i = 0; i < m_top.size(); ++i)
    {
        if (i == m_headerChunk)
            writeChunk(body, IdHeader, header());
        else if (i == m_objChunk)
        {
            std::vector<uint8_t> list;
            for (const Entry& e : m_entries)
            {
                if (!e.object)
                {
                    writeChunk(list, e.chunk.id, e.chunk.data);
                    continue;
                }
                const Object& o = m_objects[e.index];
                std::vector<uint8_t> inner;
                for (const Chunk& sub : o.chunks)
                    writeChunk(inner, sub.id, sub.id == IdObjectData && sub.data.empty() ? writeTags(o.tags) : sub.data);
                writeChunk(list, IdObject, inner);
            }
            writeChunk(body, IdObjects, list);
        }
        else
            writeChunk(body, m_top[i].id, m_top[i].data);
    }
    std::vector<uint8_t> out;
    writeChunk(out, IdPackage, body);
    return out;
}

void Package::rename(const std::string& name)
{
    const size_t slash = m_path.rfind('\\');
    m_path = (slash == std::string::npos ? std::string() : m_path.substr(0, slash + 1)) + name;
    m_name = name;
    m_renamed = true;
}

const Tag* Package::findTag(size_t object, uint16_t id) const
{
    for (const Tag& t : m_objects[object].tags)
    {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

Tag* Package::findTag(size_t object, uint16_t id)
{
    return const_cast<Tag*>(static_cast<const Package*>(this)->findTag(object, id));
}

bool Package::hasTag(size_t object, uint16_t id) const
{
    return findTag(object, id) != nullptr;
}

uint32_t Package::tag32(size_t object, uint16_t id) const
{
    const Tag* t = findTag(object, id);
    return t && t->data.size() >= 4 ? get32(t->data.data()) : 0;
}

bool Package::setTag32(size_t object, uint16_t id, uint32_t value)
{
    Tag* t = findTag(object, id);
    if (!t || t->data.size() < 4)
        return false;
    set32(t->data, 0, value);
    return true;
}

std::vector<uint8_t>* Package::tagData(size_t object, uint16_t id)
{
    Tag* t = findTag(object, id);
    return t ? &t->data : nullptr;
}

std::vector<size_t> Package::find(uint32_t hash) const
{
    std::vector<size_t> out;
    for (size_t i = 0; i < m_objects.size(); ++i)
    {
        if (tag32(i, TagHash) == hash)
            out.push_back(i);
    }
    return out;
}

uint32_t Package::type(size_t object) const { return tag32(object, TagType); }
uint32_t Package::hash(size_t object) const { return tag32(object, TagHash); }
uint32_t Package::guid(size_t object) const { return tag32(object, TagParams); }
uint32_t Package::parent(size_t object) const { return tag32(object, TagParent); }

uint32_t Package::flags(size_t object) const
{
    const Tag* t = findTag(object, TagParams);
    return t && t->data.size() >= 12 ? get32(t->data.data() + 8) : 0;
}

void Package::setFlags(size_t object, uint32_t flags)
{
    Tag* t = findTag(object, TagParams);
    if (t && t->data.size() >= 12)
        set32(t->data, 8, flags);
}

std::vector<size_t> Package::subtree(size_t object) const
{
    std::vector<size_t> out = { object };
    for (size_t at = 0; at < out.size(); ++at)
    {
        const uint32_t id = guid(out[at]);
        for (size_t i = 0; i < m_objects.size(); ++i)
        {
            if (hasTag(i, TagParent) && parent(i) == id && std::find(out.begin(), out.end(), i) == out.end())
                out.push_back(i);
        }
    }
    return out;
}

size_t Package::hide(uint32_t hash)
{
    size_t count = 0;
    for (size_t root : find(hash))
    {
        for (size_t i : subtree(root))
        {
            if (!(flags(i) & FlagHidden))
            {
                setFlags(i, flags(i) | FlagHidden);
                ++count;
            }
        }
    }
    return count;
}

uint32_t Package::buttonCount() const
{
    uint32_t count = 0;
    for (size_t i = 0; i < m_objects.size(); ++i)
        count += (flags(i) & FlagButton) ? 1 : 0;
    return count;
}

size_t Package::clone(size_t object, uint32_t newHash)
{
    const std::vector<size_t> source = subtree(object);
    uint32_t next = 0;
    for (size_t i = 0; i < m_objects.size(); ++i)
        next = std::max(next, guid(i) + 1);
    std::vector<std::pair<uint32_t, uint32_t>> ids;
    for (size_t i : source)
        ids.push_back({ guid(i), next++ });
    size_t last = 0;
    for (size_t e = 0; e < m_entries.size(); ++e)
    {
        if (m_entries[e].object && std::find(source.begin(), source.end(), m_entries[e].index) != source.end())
            last = e;
    }
    std::vector<Entry> added;
    size_t first = m_objects.size();
    uint32_t buttons = 0;
    for (size_t n = 0; n < source.size(); ++n)
    {
        Object copy = m_objects[source[n]];
        m_objects.push_back(copy);
        const size_t at = m_objects.size() - 1;
        Tag* params = findTag(at, TagParams);
        set32(params->data, 0, ids[n].second);
        if (n == 0)
        {
            set32(params->data, 4, newHash);
            setTag32(at, TagHash, newHash);
        }
        if (Tag* p = findTag(at, TagParent))
        {
            for (const auto& id : ids)
            {
                if (get32(p->data.data()) == id.first)
                    set32(p->data, 0, id.second);
            }
        }
        buttons += (flags(at) & FlagButton) ? 1 : 0;
        added.push_back({ true, Chunk(), at });
    }
    m_entries.insert(m_entries.begin() + long(last + 1), added.begin(), added.end());
    if (!hasTag(first, TagParent))
        ++m_topLevel;
    for (Entry& e : m_entries)
    {
        if (!e.object && e.chunk.id == IdButtons && e.chunk.data.size() >= 4 && buttons)
            set32(e.chunk.data, 0, get32(e.chunk.data.data()) + buttons);
    }
    return first;
}

}}
