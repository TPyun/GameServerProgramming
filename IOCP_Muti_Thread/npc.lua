myid = -1;

function set_id(x)
   myid = x;
end

function npc_talk(user_id)
   API_SendMessage(myid, user_id, "What's up dude?");
end
