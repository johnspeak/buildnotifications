module.exports = function (req, res, next) {
  var userName = req.body.user_name;
  var words = req.body.text;
  var channel = req.body.channel_name;
  var trigger = req.body.trigger_word;
  var mqtt = require('mqtt')
  var client  = mqtt.connect('mqtt:111.22.33.44')
  var botPayload
  var words = words.toLowerCase();
  var splunkBunyan = require("../index");
  var bunyan = require("bunyan");

  ...


if((words.indexOf("i am working on it") !=-1)  || (words.indexOf("im working on it") !=-1) || (words.indexOf("Im workin on it") !=-1) ||(words.indexOf("I'm workin on it") !=-1) || (words.indexOf("im workin on it") !=-1) || (words.indexOf("i'm working on it") !=-1)){
  client.on('connect', function () {
  client.subscribe('siren/team01/status')
  client.publish('siren/team01/status', 'off')
  client.end()})
  botPayload = {
text : 'thank you, ' + userName + '\nyour response has been noted and the lights are now off'
      };
  if (userName !== 'DevOps') {
    return res.status(200).json(botPayload);
  } else {
    return res.status(200).end();
  }
}