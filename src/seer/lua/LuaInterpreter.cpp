#include "LuaInterpreter.h"
#include <fmt/format.h>

#include <lua.h>
#include <seer/Log.h>

namespace seer {

void LuaInt::push(lua_State* l) {
    lua_pushinteger(l, _value);
}

LuaInt::LuaInt(int value) : _value(value) {}

void LuaString::push(lua_State* l) {
    lua_pushstring(l, _value.c_str());
}

LuaString::LuaString(std::string value) : _value(value) {}

void LuaBool::push(lua_State* l) {
    lua_pushboolean(l, _value);
}

LuaBool::LuaBool(bool value) : _value(value) {}

void LuaTable::push(lua_State* l) {
    lua_newtable(l);
    for (auto& [key, value] : _fields) {
        key->push(l);
        value->push(l);
        lua_settable(l, -3);
    }
}

void LuaTable::insert(LuaObjectS key, LuaObjectS value) {
    _fields.push_back({key, value});
}

LuaThread::~LuaThread() {
    lua_close(_state);
}

LuaThread::LuaThread() {
    _state = luaL_newstate();
    luaL_openlibs(_state);
}

bool LuaThread::execTop() {
    if (auto err = lua_pcall(_state, 0, 1, 0); err) {
        log_infof("lua_pcall error: {}", err);
        return false;
    }
    return true;
}

bool LuaThread::pushScript(const std::string& text) {
    if (auto err = luaL_loadbuffer(_state, text.c_str(), text.size(), ""); err) {
        log_infof("luaL_loadbuffer error: {}", err);
        return false;
    }
    return true;
}

void LuaThread::push(LuaObject& object) {
    object.push(_state);
}

void LuaThread::setGlobal(const std::string& name, LuaObject& value) {
    value.push(_state);
    lua_setglobal(_state, name.c_str());
}

bool LuaThread::popBool() {
    return lua_toboolean(_state, 1);
}

} // namespace seer
