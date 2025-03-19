# Chat-Moderation-System
The system simulates a chat management and moderation framework where different entities, such as moderator, and groups, are represented as processes. Within this system, each group process corresponds to a chat group (similar to a WhatsApp group) consisting of multiple users (also processes) who can interact with each other.
To simulate this interaction, each group process will spawn one or more child processes, each
representing an individual user within the group. Users will send messages to their respective group
processes, which will monitor the content for restricted words—terms Zachary deems unacceptable.
Every time a message is sent to a group, the moderator will check the message and check if the
message violates the terms. The moderator process will track the number of violations committed by
each user. If a user exceeds a predefined threshold, the moderator will instruct the respective group
process to remove/ban that user from the group.
The group process must handle the user messages and forward them to the validation process via a
message queue in the correct order based on their unique timestamps. The validation process
(validation.out) will verify that the messages are correctly ordered in ascending order of their timestamps
and ensure that messages from banned users (users who have been removed because of violations) are
properly blocked. Failure to meet these requirements will result in a failed test case, with corresponding
mark deductions.

If a user is removed/banned, the group must continue sending messages to the validation process in the
correct order, ensuring that only messages from users who are currently in the group are sent. This
implies that even if a banned user ends up sending a message to the group process, that message is
ignored and is not forwarded by the group process.
If the number of users in a group becomes less than 2, the group must be terminated.
