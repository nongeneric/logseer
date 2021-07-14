#pragma once

#include <lua.hpp>

#include <string>
#include <memory>
#include <vector>

namespace seer {

class LuaObject {
    friend class LuaTable;
    friend class LuaThread;
protected:
    virtual void push(lua_State* l) = 0;
public:
    virtual ~LuaObject() = default;
};

using LuaObjectS = std::shared_ptr<LuaObject>;

class LuaInt : public LuaObject {
    int _value;
protected:
    void push(lua_State* l) override;
public:
    LuaInt(int value);
};

class LuaString : public LuaObject {
    std::string _value;
protected:
    void push(lua_State* l) override;
public:
    LuaString(std::string value);
};

class LuaBool : public LuaObject {
    bool _value;
protected:
    void push(lua_State* l) override;
public:
    LuaBool(bool value);
};

class LuaTable : public LuaObject {
    std::vector<std::tuple<LuaObjectS, LuaObjectS>> _fields;
protected:
    void push(lua_State* l) override;
public:
    void insert(LuaObjectS key, LuaObjectS value);
};

class LuaThread {
    lua_State* _state = nullptr;
public:
    ~LuaThread();
    LuaThread();
    bool execTop();
    bool pushScript(const std::string& text);
    void push(LuaObject& object);
    void setGlobal(const std::string& name, LuaObject& value);
    bool popBool();
};

} // namespace seer
