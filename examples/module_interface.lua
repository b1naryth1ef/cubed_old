--[[
  This is an example of mod interactions
--]]

require 'cubed'

local mymod = cubed.mod.create("mymod")
mymod.items.create("iron dust", {
  "smelt": {
    "needs": 1,
    "provides": 1,
    "result": cubed.items.find("iron")
  }
})

mymod.blocks.create("pulverizer")

local itable = mymod.createInterfaceTable("pulverizer")
itable.addBind()

 