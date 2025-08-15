# this is discord bot code that
# 1. wait for a message on its channel
# 2. wait for specic command
# 3. reads data from other channel and creates a excel-convertable .txt file based on command

import discord                  # to interact with Discord API
from datetime import datetime   # for easy to use dates
from datetime import timezone   # for utc to local timezone conversion 
import os                       # for managing files
from dotenv import load_dotenv  # for using .env file to store bot token

# load variables from hidden .env file
load_dotenv()
# bot's token (!keep secret!)
TOKEN = os.getenv("TOKEN")
# Discord channel IDs
TEST_CHL = 1401922482713264160 
FIELD_CHL = 1403400570806468718 
FILES_CHL = 1403400656017948793 

# intents your bot needs
intents = discord.Intents.default()
intents.message_content = True 

client = discord.Client(intents=intents) #client instance of the bot

@client.event
async def on_message(message):   
    if message.channel.id != FILES_CHL: # ignore messages outside this channel
        return 
    
    if message.author == client.user: # ignore messages sent by bot
        return
    
    bot_channel = client.get_channel(FILES_CHL) #define bot channel
    
    if message.content.startswith("Create file:"):  #if recent message starts with "Create file:" continue
        lines = message.content.split('\n')
        if len(lines) == 5:         # if there are 5 total lines in the message continue 
            channel_name = lines[1] # channel bot will read from 
            name = lines[2]         # name to look for in data 
            start_str = lines[3]    # start time of messages to read from 
            end_str = lines[4]      # end time when to stop reading messages 

        else: 
            await bot_channel.send("Incorrect number of lines.")
            return
    else: 
        await bot_channel.send("Read channel description to find how to correctly format a command to create a .txt data file.")
        return 
    
    try: #if dates are formated correctly (MM-DD-YYYY HH:MM:SS) continue 
        start_time = datetime.strptime(start_str, "%m-%d-%Y %H:%M:%S")
        end_time = datetime.strptime(end_str, "%m-%d-%Y %H:%M:%S")
    except ValueError: 
        await bot_channel.send("Incorrect time format.")
        return
    
    if start_time == end_time: # if start and end date are the same stop 
        await bot_channel.send("Start and end time cannot be the same.")
        return
    
    if start_time > end_time:  # if start date is after end date stop 
        await bot_channel.send("Start and end time must be cronilogical.")
        return
    
    # if channel name is "testing" or "field data" set up reading from that channel 
    if channel_name == "testing": 
        read_channel = client.get_channel(TEST_CHL)
    elif channel_name == "field-data":
        read_channel = client.get_channel(FIELD_CHL)
    else:
        await bot_channel.send("Incorrect channel name.")
        return 
    
    file_content = []      # list of data lines that will be transfered to file 
    num_channel_msgs = 0   # number of available messages between start and end date 
    msgs_with_name = 0     # number of messages found between start and end date that contain name
    msgs_formatted = 0     # number of messages succesfully formatted 
    
    # read from desired channel between start and end date (discord uses UTC time) 
    # and store all messages that contain ~name or ^name
    async for message in read_channel.history(after=start_time.astimezone(timezone.utc), before=end_time.astimezone(timezone.utc), oldest_first=True, limit=None):
        num_channel_msgs += 1 
        
        if message.content.startswith(f"~{name}"): 
            msgs_with_name += 1
            try: 
                #add Discord date and time 
                file_content.append(
                    f"{message.created_at.replace(tzinfo=timezone.utc).astimezone().strftime('%m-%d-%Y %H:%M:%S')} "
                    f"{message.content}"
                )
            except: 
                msgs_formatted -= 1
       
        if message.content.startswith(f"^{name}"):    
            msgs_with_name += 1          
            # if message countains dash (-) add Discord date and time headings 
            try: 
                file_content.append(f"Discord+Date Discord+Time {message.content}")
            except: 
                msgs_formatted -= 1

    
    time_zone = datetime.now().astimezone().strftime("%Z") or datetime.now().astimezone().tzname() or "LOCAL"
    
    # format stored messages 
    for i in range(len(file_content)):
        try: 
            # clear ~name or ^name
            file_content[i] = file_content[i].replace(f"~{name} ", "").replace(f"^{name} ", "")
            # replace any spaces or consecutive spaces with tab character
            file_content[i] = '\t'.join(file_content[i].split()) 
            #replace plus with space
            file_content[i] = file_content[i].replace("+", " ")
            # replace (newline) with newline and tab characters 
            file_content[i] = file_content[i].replace("(newline)", f"\n{time_zone}\t{time_zone}\t")
        except: 
            msgs_formatted -= 1
            
    # send this report message with created file 
    await bot_channel.send(
        f"{num_channel_msgs} messages found\n"
        f"{msgs_with_name} messages found with DRIFTER_NAME \"{name}\"\n"
        f"{msgs_formatted} messages formatted to add Discord time, clear DRIFTER_NAME \"{name}\", and add tabs or newlines"
    )          
    
    # file name = name_YYYY-MM-DD(start)_YYYY-MM-DD(end)
    file_name = (
        f"{name}_"
        f"{start_time.strftime('%m%d%Y')}_{start_time.strftime('%H%M%S')}_"
        f"{end_time.strftime('%m%d%Y')}_{end_time.strftime('%H%M%S')}.txt"
    )
    
    # write formatted list to a temporary file 
    with open(file_name, "w", encoding="utf-8") as f:
        for line in file_content:
            f.write(line + "\n")
    
    # send file to channel
    await bot_channel.send(file=discord.File(file_name))

    # delete file from the folder the code runs on
    if os.path.exists(file_name):
        os.remove(file_name)

# run program 
client.run(TOKEN)
#Ctrl + C in terminal ends task 