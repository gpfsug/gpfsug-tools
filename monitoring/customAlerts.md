# Custom Alerts

Example usage of custom alerts in GUI and mmhealth:


Ref: https://www.ibm.com/support/knowledgecenter/STXKQY_5.0.2/com.ibm.spectrum.scale.v5r02.doc/bl1adv_createuserdefinedevents.htm?view=embed#bl1adv_createuserdefinedevents

Note:

    While the severity can be set as INFO, WARNING, or ERROR, event_type must   be set to INFO.
	
    You can fill the cause, description, and message with descriptive tex      t. The message can include place holders in the form of n, where n is an integer value. These placeholders are filled with arguments that are provided at the time of raising the event. A new line (\n) must not be included in the message.

```
cat > /var/mmfs/mmsysmon/custom.json << 'EOF'
{
  "event_1":{
       "cause":"cause text",
        "user_action":"user_action text",
        "scope":"NODE",
        "entity_type":"NODE",
        "code":"ev_001",
        "description":"description text",
        "event_type":"INFO",
        "message":"message with argument {0} and {1}",
        "severity": "INFO"
  },
  "event_2":{
       "cause":"cause text",
        "user_action":"user_action text",
        "scope":"NODE",
        "entity_type":"NODE",
        "code":"ev_002",
        "description":"description text",
        "event_type":"INFO",
        "message":"message with argument {0} and {1}",
        "severity": "WARNING"
  },
  "event_3":{
       "cause":"cause text",
        "user_action":"user_action text",
        "scope":"NODE",
        "entity_type":"NODE",
        "code":"ev_003",
        "description":"description text",
        "event_type":"INFO",
        "message":"message with argument {0} and {1}",
        "severity": "ERROR"
  }
}
EOF

ln -s /var/mmfs/mmsysmon/custom.json /usr/lpp/mmfs/lib/mmsysmon/custom.json
mmsysmoncontrol restart
```

Test it:
```
# mmhealth event show event_1
Event Name:              event_1
Event ID:                ev_001
Description:             description text
Cause:                   cause text
User Action:             user_action text
Severity:                INFO
State:                   Not state changing

# mmsysmonc event custom event_1 "arg1,arg2"
Event event_1 raised
# mmsysmonc event custom event_2 "arg1,arg2"
Event event_2 raised
# mmsysmonc event custom event_3 "arg1,arg2"
Event event_3 raised

# mmhealth node eventlog --hour|tail
2018-12-06 13:44:16.769246 CET        event_1                   INFO       message with argument arg1 and arg2
2018-12-06 13:44:19.978106 CET        event_2                   WARNING    message with argument arg1 and arg2
2018-12-06 13:44:22.900256 CET        event_3                   ERROR      message with argument arg1 and arg2

```

event_2 & event_3 is shown under "current issues", event_1 under "issues" or all events..

