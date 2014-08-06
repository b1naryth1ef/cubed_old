require 'cubed'

local mymod = cubed.mod.create("mymod")

local dirt = mymod.createType("dirt") -- alias for cubed.types.create
local stick = mymod.createItem("stick") -- alias for cubed.items.create

stick.onClickLeft = function (ctx)
    blk = ctx.player.getFocusedBlock()
    if not blk then return end

    blk.set(dirt)

    eve = ctx.newChildEvent(blk.onBlockPlace)
    eve.trigger()
end

cubed.commands.create("freeze", function (ctx)
    ctx.player.bind(ctx.player.onMove, function (ctx2)
        ctx2.cancel()
    end, "freeze_command_bind")
end)

cubed.commands.create("unfreeze", function (ctx)
    if ctx.player.hasBind("freeze_command_bind") then
        ctx.player.unbind("freeze_command_bind")
    end
end)

