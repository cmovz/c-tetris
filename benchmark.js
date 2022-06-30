const fs = require('fs');

const dataStr  = fs.readFileSync('./results.txt', {encoding:'utf-8'});
const aiRegex  = /AI scored       : (\d+)/g;
const saiRegex = /Simple AI scored: (\d+)/g;

let ai = {
  wins: 0,
  losses: 0,
  draws: 0,
  results: [],
  sortedResults: [],
  totalPoints: 0,
};

let sai = {
  wins: 0,
  losses: 0,
  draws: 0,
  results: [],
  sortedResults: [],
  totalPoints: 0,
}

while (true) {
  let res = aiRegex.exec(dataStr);
  if (!res) {
    break;
  }
  ai.results.push(+res[1]);
  ai.totalPoints += +res[1];
}
ai.sortedResults = [...ai.results].sort();

while (true) {
  let res = saiRegex.exec(dataStr);
  if (!res) {
    break;
  }
  sai.results.push(+res[1]);
  sai.totalPoints += +res[1];
}
sai.sortedResults = [...sai.results].sort();

ai.results.forEach((_, i) => {
  if (ai.results[i] > sai.results[i]) {
    ai.wins++;
    sai.losses++;
  } else if (ai.results[i] < sai.results[i]) {
    ai.losses++;
    sai.wins++;
  } else {
    ai.draws++;
    sai.draws++;
  }
});

let mid0 = Math.floor((ai.results.length - 1) / 2);
let mid1 = Math.floor(ai.results.length / 2);
ai.medianPoints = Math.floor(
  (ai.sortedResults[mid0] + ai.sortedResults[mid1]) / 2
);
sai.medianPoints = Math.floor(
  (sai.sortedResults[mid0] + sai.sortedResults[mid1]) / 2
);

ai.maxPoints = Math.max(...ai.results);
ai.minPoints = Math.min(...ai.results);
sai.maxPoints = Math.max(...sai.results);
sai.minPoints = Math.min(...sai.results);

console.log(`
AI stats:
wins:   ${ai.wins}
losses: ${ai.losses}
draws:  ${ai.draws}
max points: ${ai.maxPoints}
min points: ${ai.minPoints}
total points: ${ai.totalPoints}
median points: ${ai.medianPoints}

Simple AI stats:
wins:   ${sai.wins}
losses: ${sai.losses}
draws:  ${sai.draws}
max points: ${sai.maxPoints}
min points: ${sai.minPoints}
total points: ${sai.totalPoints}
median points: ${sai.medianPoints}
`)