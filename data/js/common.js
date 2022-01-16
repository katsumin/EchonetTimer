function registerObject(url, obj){
	let data = JSON.stringify(obj);
	$.ajax({
		url: url,
		cache: false,
		type: "post",
		data: {
			"id": obj["id"],
			"settings": data
		}
	}).done(function(response){
		console.log("done: ");
		console.log(response);
	}).fail(function(xhr){
		console.log("fail: ");
		console.log(xhr);
	}).always(function(xhr,msg){
		console.log("always: " + xhr + ", msg: " + msg);
	});
}

function requestObject(url, id, callback){
	$.ajax({
		url: url,
		cache: false,
		type: "get",
		contentType: "text/plain",
		data: id
	}).done(function(response){
		callback(response);
	}).fail(function(xhr){
		console.log("fail: ");
		console.log(xhr);
	}).always(function(xhr,msg){
		console.log("always: " + xhr + ", msg: " + msg);
	});
}

function copySelectItem(target) {
	const parent = $(target).parent().clone(false);
	parent.insertAfter($(target).parent());
	const cp = parent.parent().find(".time-value");
	cp.clockpicker();
}

function createSelectItem(selectObj, time, value, color) {
	const selectItem = $("<div></div");
	const timeObj = $("<input type='text' class='time-value'/>").appendTo(selectItem);
	timeObj.val(time);
	timeObj.clockpicker();
	selectObj.appendTo(selectItem);
	selectObj.val(value);
	const colorObj = $("<input type='color' class='select-color' />").appendTo(selectItem);
	colorObj.val(color);
	$(`
	<button class="add bi-plus-circle adddel-button"></button>
	<button class="del bi-dash-circle adddel-button"></button>
	`).appendTo(selectItem);

	return selectItem;
}

function isEmpty(object) {
	return Object.keys(object).length === 0;
}

const chartMap = {};

function createRowItem(dataObj) {
	const alias = dataObj["alias"];
	const type = dataObj["type"];
	const id = dataObj["id"];
	const table = dataObj["table"];
	let typeName = "";
	// let selectItems;
	const selectContainer = $("<div class='col-7'></div>");
	if (type === "027d01") {
		// 蓄電池
		typeName = "蓄電池";
		if (isEmpty(table)) {
			const selectObj = $(`
			<select class='mode-select'>
				<option value='0000'></option>
				<option value="da42">充電</option>
				<option value="da43">放電</option>
				<option value="da44">待機</option>
				<option value="da46">自動</option>
			</select>
			`);
			const selectItem = createSelectItem(selectObj, null, null, null);
			selectItem.appendTo(selectContainer);
		} else {
			table.forEach( function(t) {
				const selectObj = $(`
				<select class='mode-select'>
					<option value='0000'></option>
					<option value="da42">充電</option>
					<option value="da43">放電</option>
					<option value="da44">待機</option>
					<option value="da46">自動</option>
				</select>
				`);
				const selectItem = createSelectItem(selectObj, t["time"], t["value"], t["color"]);
				selectItem.appendTo(selectContainer);
			});
			}
	} else if (type === "013001") {
		// エアコン
		typeName = "エアコン";
		if (isEmpty(table)) {
			const selectObj = $(`
			<select class='mode-select'>
				<option value='0000'></option>
				<option value="8030">ＯＮ</option>
				<option value="8031">ＯＦＦ</option>
			</select>
			`);
			const selectItem = createSelectItem(selectObj, null, null, null);
			selectItem.appendTo(selectContainer);
		} else {
			table.forEach( function(t) {
				const selectObj = $(`
				<select class='mode-select'>
					<option value='0000'></option>
					<option value="8030">ＯＮ</option>
					<option value="8031">ＯＦＦ</option>
				</select>
				`);
				const selectItem = createSelectItem(selectObj, t["time"], t["value"], t["color"]);
				selectItem.appendTo(selectContainer);
			});
			}
	} else {
		return;
	}
	const parentId = "echonet-timer";
	const accordionItem = $("<div class='accordion-item'></div>").appendTo("#" + parentId);
	const idHeader = "head-" + id;
	const idCollapse = "collapse-" + id;
	const idChart = "chart-" + id; 
	const accordionHeader = $("<h2 class='accordion-header' id='" + idHeader + "'></h2>").appendTo(accordionItem);
	accordionHeader.append("<button class='accordion-button' type='button' data-bs-toggle='collapse' data-bs-target='#" + idCollapse + "' aria-expanded='true' aria-controls='" + idCollapse + "'>" + typeName + "（" + alias + "）</button>");
	// 開いたとき、別のアイテムを閉じる
	// const subItem = $("<div class='accordion-collapse collapse show' id='" + idCollapse + "' aria-labelledby='" + idHeader + "' data-bs-parent='#" + parentId + "'></div>").appendTo(accordionItem);
	// 開いたとき、別のアイテムを閉じない
	const subItem = $("<div class='accordion-collapse collapse show' id='" + idCollapse + "' aria-labelledby='" + idHeader + "'></div>").appendTo(accordionItem);

	$("<div style='display:none;' class='dataObj-id'>" + id + "</div>").appendTo(subItem);
	$("<div style='display:none;' class='dataObj-type'>" + type + "</div>").appendTo(subItem);
	const rowItem = $("<div class='row'></div>").appendTo($("<div class='accordion-body'></div>").appendTo(subItem));
	const inputAlias = $("<input type='text' class='dataObj-alias'></input>").prependTo(selectContainer);
	inputAlias.val(alias);
	selectContainer.appendTo(rowItem);
	const reflectBtnId = "reflect-" + id;
	const sendBtnId = "send-" + id;
	const reflectButton = $("<div class='col-1 button-container'><button id='" + reflectBtnId + "' class='reflect-button bi-play-btn'></button><button id='" + sendBtnId + "' class='send-button bi-box-arrow-up'></button></div>").appendTo(rowItem);
	const chartItem = $("<div class='col-4'><canvas id='" + idChart + "' width='100px' height='100px'></canvas></div>").appendTo(rowItem);

	// 初期チャート
	const chart = drawChart(idChart, selectContainer);
	chartMap[reflectBtnId] = chart;

	$("#" + reflectBtnId).click( function() {
		if (chartMap[reflectBtnId] !== null && chartMap[reflectBtnId] !== undefined) {
			chartMap[reflectBtnId].destroy();
		}
		// チャート更新
		const chart = drawChart(idChart, selectContainer);
		chartMap[reflectBtnId] = chart;
	});

	$("#" + sendBtnId).click( function() {
		const textId = subItem.find(".dataObj-id").text();
		const textType = subItem.find(".dataObj-type").text();
		const textAlias = subItem.find(".dataObj-alias").val();
		const table = extractSelectObject(selectContainer);
		const sendData = {
			id: textId,
			alias: textAlias,
			type: textType,
			table: table
		};
		console.log(sendData);
		registerObject("/register", sendData);
	});
}

function extractSelectObject(selectContainer) {
	const table = [];
	selectContainer.children("div").each( function(i, elm) {
		const t = $(elm).find(".time-value").val();
		const v = $(elm).find(".mode-select").val();
		const c = $(elm).find(".select-color").val();
		if (t !== null && t.length > 0 && v !== null && v.length > 0) {
			table.push({
				time : t,
				value : v,
				color: c
			});
		}
	});

	table.sort( function(a,b) {
		return (a.time < b.time) ? -1 : 1;
	});
	return table;
}

// "23:01" -> 23 * 60 + 1
function minuteValue(time) {
	const words = time.split(":");
	return Number(words[0]) * 60 + Number(words[1]);
}

const labelTable = {
	"da42": "充電",
	"da43": "放電",
	"da44": "待機",
	"da46": "自動",
	"8030": "ＯＮ",
	"8031": "ＯＦＦ"
};

// 
function drawChart(target, selectContainer) {
	const colors = [
	];
	const angles = [];
	const labels = [];
	let rotate = 0;

	let myDoughnutChart = null;
	const table = extractSelectObject(selectContainer);
	console.log(table);
	const length = table.length;
	// const length = selectContainer.children("div").length;
	if (length > 0) {
		const total = 24 * 60;

		for ( let i=0; i<length; i++) {
			const textFrom = table[i].time;
			const textTo = table[(i + 1) % length].time;
			const value = table[i].value;
			const color = table[i].color;
			const timeFrom = minuteValue(textFrom);
			const timeTo = minuteValue(textTo);
			if (i === 0) {
				rotate = timeFrom / total * 360;
			}
			labels.push(labelTable[value] + "(" + textFrom + "-" + textTo + ")");
			let diff = timeTo - timeFrom;
			diff = (diff >= 0 && length > 1) ?diff :diff + total;
			angles.push(diff);
			colors.push(color);
		}
		
		myDoughnutChart = new Chart( target, {
			type: 'doughnut',
			data: {
				datasets: [{
				  data: angles,
				  backgroundColor: colors
				}],
				labels: labels
			},
			options: {
				rotation: rotate,
				"anamation.animateAutoScale": true
			}
		}); 
	}

	return myDoughnutChart;
}