// Device
var address = "192.168.137.187";
var device = new Device(address);

var final_transcript = '';
var recognizing = false;

if ('webkitSpeechRecognition' in window) {

  var recognition = new webkitSpeechRecognition();

  recognition.continuous = true;
  recognition.interimResults = false;

  recognition.onstart = function () {
    recognizing = true;
  };

  recognition.onerror = function (event) {
    console.log(event.error);
  };

  recognition.onend = function () {
    recognizing = false;
  };

  recognition.onresult = function (event) {
    var interim_transcript = '';
    for (var i = event.resultIndex; i < event.results.length; ++i) {
      if (event.results[i].isFinal) {
        final_transcript += event.results[i][0].transcript;
      } else {
        interim_transcript += event.results[i][0].transcript;
      }
    }
    final_transcript = capitalize(final_transcript);
    //final_span.innerHTML = linebreak(final_transcript);
    //interim_span.innerHTML = linebreak(interim_transcript);
    executeTranscription(final_transcript);
  };
}

var two_line = /\n\n/g;
var one_line = /\n/g;
function linebreak(s) {
  return s.replace(two_line, '<p></p>').replace(one_line, '<br>');
}

function capitalize(s) {
  return s.replace(s.substr(0, 1), function (m) { return m.toUpperCase(); });
}

function startDictation(event) {
  if (recognizing) {
    recognition.stop();
    return;
  }
  final_transcript = '';
  recognition.lang = 'en-US';
  recognition.start();
  final_span.innerHTML = '';
  interim_span.innerHTML = '';
}

function executeTranscription(transcription){
  console.log(transcription.toLowerCase());
  var words = transcription.toLowerCase().split(' ');
  for(let i = 0; i < words.length; i++){
    if(words[i] === 'forward'){
      console.log('Moving Forward');
      device.callFunction("forward");
    } else if(words[i] === 'left'){
      device.callFunction("left");
    } else  if(words[i] === 'right'){
      device.callFunction("right");
    } else if(words[i] === 'backward'){
      device.callFunction("backward");
    } else if(words[i] === 'stop'){
      device.callFunction("stop");
    }
  }
}