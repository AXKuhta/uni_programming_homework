#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ascii_lib.c"

// Печать без буферизации
// Под Windows проблем с упорядоченностью вывода не было, а под Linux появились
#define print(x) if (write(1, x, strlen(x))) {}

//
// Float128:
// Знак			1 бит
// Экспонента	15 бит
// Мантисса 	112 бит
//

struct float128_s {
	uint64_t hi;
	uint64_t lo;
};

typedef	struct float128_s float128_t;


// Получить знак
// Есть минус		1
// Нет минуса		0
int float_128_get_sign(float128_t* src) {
	if (src->hi & 0x8000000000000000) {
		return 1;
	} else {
		return 0;
	}
}

// Получить экспоненту
uint16_t float128_get_exp(float128_t* src) {
	return ((src->hi >> 48) & 0x7FFF);
}

// Right Hand Side Mask; Маска по правой стороне
//
// Багованные варианты:
// ((1ull << x) - 1)		Не будет работать при x = 64
// (~0ull >> (64 - x))		Не будет работать при x = 0
//
// Вернёт:
// 0        0000
// 1        0001
// 2        0011
// 3        0111
// И т.д. до 64
#define RHS_MASK(x) (x > 0 ? (~0ull >> (64 - x)) : 0)

// Сложить два Float128
// a = a + b
void float128_add(float128_t* src_a, float128_t* src_b) {
	uint16_t exp_a = float128_get_exp(src_a);
	uint16_t exp_b = float128_get_exp(src_b);

	if (exp_b > exp_a) {
		print("Not OK\n");
		exit(-1);
	}

	// Проблема: битовые операции сломаются, если precision_delta > 64
	// На такой случай нужна альтернативная обработка, где hi = 0, lo = hi >> delta
	uint16_t precision_delta = exp_a - exp_b;

	//printf("\nPrecision delta: %d\n", precision_delta);

	// Будем корректировать точность b во временных переменных
	uint64_t temp_hi = src_b->hi & 0x0000FFFFFFFFFFFF; // Отпилить знак/экспоненту -- они уже были сохранены выше
	uint64_t temp_lo = src_b->lo;


	//printf("HI: 0x%016I64x\n", temp_hi);
	//printf("LO: 0x%016I64x\n", temp_lo);


	// Принудительно выставить бит, который обозначается экспонентой
	temp_hi |= 0x0001000000000000;

	// Биты, которые мигрируют из hi в lo
	uint64_t migrated = temp_hi & RHS_MASK(precision_delta);

	// Сдвинуть LO
	temp_lo >>= precision_delta;

	// Вставить мигрировавшие биты
	temp_lo |= (migrated << (64 - precision_delta));

	// Сдвинуть HI
	temp_hi >>= precision_delta;

	//printf("MASK: 0x%016I64x\n", RHS_MASK(precision_delta));
	//printf("HI: 0x%016I64x\n", temp_hi);
	//printf("LO: 0x%016I64x\n", temp_lo);


	// Добавить в hi
	src_a->hi += temp_hi;

	// Если при сложении lo произойдёт переполнение, то инкрементнуть hi
	if (src_a->lo + temp_lo < src_a->lo)
		src_a->hi++;
	
	// Добавить в lo
	src_a->lo += temp_lo;

	// Проверить переполнение экспоненты
	// Если переполнилось, то сдвинуть мантиссу вправо
	// Есть ли какой-нибудь более лаконичный способ двигать число, размазанное по двум переменным?
	if (float128_get_exp(src_a) > exp_a) {
		src_a->lo >>= 1;
		src_a->lo |= (src_a->hi << 63);

		uint64_t stay = src_a->hi & 0xFFFF000000000000;
		uint64_t move = src_a->hi & 0x0000FFFFFFFFFFFF;

		src_a->hi = stay | (move >> 1);
	}
}

// Напечатать Float128
// Хорошая производительность для чисел >1.0
// Ужасная производительность для чисел <1.0
void print_float128(float128_t* src) {
	if (float_128_get_sign(src))
		print("-");

	uint64_t exp = float128_get_exp(src);

	int high_bit = exp - 16384 + 1;
	int mantissa_size = 112 - high_bit;

	if (mantissa_size < 0)
		mantissa_size = 0;

	//
	// =================
	// Вывод целой части
	// =================
	//
	if (high_bit >= 0) {

		// Подвести вес разряда к рабочему диапазону
		// Начать с единицы
		// 1 + 1 = 2
		// 2 + 2 = 4
		// 4 + 4 = 8
		// 8 + 8 = 16
		// И т.д.
		av_t base = av_from_string("1");

		for (int i = 0; i < high_bit - 112; i++)
			base = ascii_add(&base, &base);

		// Начать считать
		av_t num = av_from_string("0");

		// Но ограничить high_bit
		if (high_bit > 112)
			high_bit = 112;


		uint64_t slice = 0;
		int roi = high_bit;

		if (high_bit > 48) {
			roi = high_bit - 48;
			slice = src->lo >> (64 - roi);
		} else {
			slice = src->hi;
		}

		for (int i = 0; i < high_bit; i++) {
			// Переключить кусок, если необходимо
			if (i == roi) slice = src->hi;

			if (slice & 1)
				num = ascii_add(&num, &base);

			base = ascii_add(&base, &base);
			slice = slice >> 1;
		}

		// И финальный бит
		// Затем напечатать
		num = ascii_add(&num, &base);

		print_av(&num);
	} else {
		print("0");
	}

	//
	// ===================
	// Вывод дробной части
	// ===================
	//

	if (mantissa_size > 0) {

		// Подвести вес разряда к рабочему диапазону
		av_t base;

		if (high_bit >= 0) {
			base = av_from_string("0000000000000000000000000000000001925929944387235853055977942584927318538101648215388195239938795566558837890625");

			for (int i = 0; i < 112 - mantissa_size; i++)
				base = ascii_add(&base, &base);
		} else {
			base = av_from_string("000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000647517511943802511092443895822764655249956933803468100968988438919703954012411937101767149127664994025587814147684811967658721988638254204668511007197261798304279271075133493441673462563847174023944852650555399039145555625217114806807082203468825698247627282878910302835733756134803106238656459263982622699190790786766326206571121158306465719606830833284523445306976052648944766096457931375140340263180435003994887007525564871336806611787940315576671330346743493706240941168521513760733313942284383505166898356716719680264295235350407971434710386053778289370021552116866771104295061002188151362798642946170043333920193539749882518433538551489284433993085296783844868212550230411412215304594646546308476411017447873704433531238966148363921055394341147654478626139787506419145122676761462589279036996141506960698000708050278864891997591680187822200225238172304723097187657099542882121928159654763302353878313718364639695283153055106868341959673537408674629052586799621690532336531988517260995682762551103633247835322894763188005068455915060831898652154013606127377149339041278475655210389751852776415877875933388071488227963332382639731237540692703944652530644783851437150365498785517230630758415545982670709590961775252680040132699082872163372677044091543909267790664679857352419911664826420692045116013363507136177381212171652817814824072078322673654962373613527499030913814318324837195987597880502750092869380807333695575275908844286560007519988899832388064354967005550542136243510857273208732298202718777245923122898947237295186432109926187878108433025298833392136299896330372243105974196876345240695383501512053377338528990471084362840270027470287047407382524882888998499428460760484721179422121072991315982901421716800446104953266269654291318362815719956164194992393662936899285662138308454532763697226554235534174985566302000488731683521615676722305379981106725891380846999423260785885452406443214577592214453570123646898570521297704547948951569000938892362558068578870760587734385766917089465679742336849032940725116039335604882060708508906902541358426981673974721123951355041438311750882676314351286200649705804114529609789291799993557705323425269869193379272475368218453768283556282993863000483838436394424313762286491825747425628012729111696505060017812047772078474218420537619597583167772176474004453662464576447138157145238183012539876515208328748311027939469785591571398193997068459919959486300723062407678613961755463145073499519008193218761681423463273877822443325646373834643527490571092558110606127384097559293176126679926558776546894676433708519236935831629845745000880942858692150474373087389571047461448000372714616234985833329151655216254537031220039084158830948097327463164627359739024947774665713189119648239867011879791009891889968948219695666063822573196069354276396430230899969295709106130018364832978518279330140726720004638304300523675997738760140776911038329572991865452093778558639063315702034763281210872863858981328256952957676936717332476408528570530831295495740155824716873993144656942281872348247109737436210527927855857074627816833159690379952795900167485615556461982140704479972002062064223324605784823647604754076284978832543563059242019449044353942899089948865589018388678865051859554218025793920053964504180838776023818956242835177463244062261805691313749982002192473317366803680648573344356703961386603717615595525382754847336044051704942269607550118329257306325061302116750385856108223837443919341323147260807288873332651285328780677338907329929737276024439735579150200465445811872803751032903412064303018234671601197499671959893427763529776295883724738936690976336749368229943036266899619393540522622118028046269135391228245570175981003711643996888363959610892641352882620050251155107166154107124189231907081299426951808638529599238102595717299239931214141421023554770700855559004950929404833117825888564525429573783492759033277310584382679775654003981029502169903778822820507214673809432026198082325209685287567059136619455659554947036209031723334912816047562629324653843003922646387725351785506298189005871241586657872769436287780507859411593904992269721568913904288447582979820139922184333984314433432975097626505437589863708761940677635786234159953958577217336256992972321731385621861073907991169392839557335410769520792438944304773598609705711710276164739858374670406622219248962849157924186151222845794272767765621706806030406343185944499909287309554680803589407119387267692420789359505133406787086697858524106329839944880166497950357625873085313776265671593267458487840876507434871535080821116048803756737008244959691404437863138435157451457440726626860151841812114063901777295075543982235214260546563975633093718543481277643745517005270425487368952598944565278155947464422753768496432162594620064734772610046887544579759229119236372464110060257172396010438972756148761621389388015869926156535386039426841251947825075588212150235085577754898735495718761136464434778346062391856979035771207870803007172986214720411906200314578305765796433562049068665477777585777241723065400566938013727218865010450285956328253564273453343056728731949830452891587663837029660720965472724681012074914701157984600768687687234897876761019701816744410153265650069958640767440432524938018706695622887762120628113014103533992925989508721680997635603930417632461505122386364213298749398928209613202371962867517965979309944112423418135458308263630611543033524825663659554275306357794617739929433631845961175483118965706507967993492812911329518941166074930622674706175331676048029076084642134755755360718525446251499163478916321559255456528862502142906459421744403443509463548980129463604421714189412860856020083794290110401471378303904658722883388803270951373270010166700283388501014240762023343810686108706205590687487289349113367848425972082088639772443893994432644994251669662868351003042623725776542626953259524818328961160237395230507382702942583848100744336148117203366419989361493262355437810131335392569916276758180944417495729353264103411236716394047249645482664549660826849143597533075479202247341325464163530913136536001391490951996323604695289708934701613673898413313379087410114316869102214191896779787747645010058506734876618322499081425361076359297473178426383285367504534324653325496613131153141106847871042978679181140465868080558810576851829209812925916415115312661534149837932435395345072387409240115201578762887939060378505752237508832277936400701843358254367309099083335229630229039016533664735524236598290367064856372723928354724120202902190199349345323140754394253337430312885554471885468113381870371730961599956361702606936020815639321492433385603772677125985793129848790900678298719592683591053727921358290566738493574587148388072495968537728445545132500938935911535634185974754830207571561443987671230411399567925090745488389530239663238165323053203269548175123194006285602445665638122071341343132544448375753426244933967514721424307914459865880820254892631212059669786335655472234811419496940917407245537222135482618988202392252487605493590592945425892550538789803952832907668955110893234285624369799963793822806866559534585597821542027455856236859023646771188477051266632830675628655934857463627291641433234999222527737907195209523444304182005485497032393140407761032373098891488918835612753391407485610708647983158473975562210683497136899733424643226401275726162895421362617779712728981835755892659191117537613455473736767294122441696659914452068803303238849446459748160068423317996968871337715396910701482058650018789002841975996662912589690385972448201855156736845933943728202747316371363055437014651828106804970184957508974343234930884928145618274502701184565362629078073829918130577248200125099176104754334479045462594315608504466110357651252336616135109964075113538514830591398788900869800923437094189134609830643793766122717238650698063690374354128523560578016721600892082861084206652209658950042094060640569266410990506323486611707751630869417570912321947628323029020749755515946714997934702020967350049665102355447500505399129537443072852477121243510738266431884270319781190107502703670304141931522915626327970886506400447339240171081753304021015275796669141647183636149854624918086604903973829426586386492540613575242853715988013206104282809077273310356143702107093883552048498756478490235782664500618538484729971151304255891311655067984298178573771372579004512809441167626485857320152907367807339638751926837379858254826386859211966609738118925524719064135219544308143294598190122371674571254374484714650301246008521614833580761962452839606901037087198164271275593296948450115518448745459784637578259880970809689364543985647863450814685906597817935100758074492897411452978821522648460182497414010735275877637899380000847946364045977378810603922164752791561963837589727965394686705736633783472126550008103569537295818707736763384842635134595953764413125539388694982804096611276801374567147662222197317229902132272383068434505481815748450176011373429966229824203945969448407467180888012626196399930140083357536658722023918044388594863491143267245490957847958697606459812663317783501182481204044009052973691543656433031778434326094141919900799456555633619565499476776839164184316729124258788240009120812434866999634307006102673255458742877997643951225916173703633584768234672853954977849338887052810296982292397645402247732207520490542037782380087654072456078224254004335849430575864839007138855614295499144089502603434229775462438747273269742326021101473172919613917251405395259142824788738324334475538910449603797536067722476559121197038221222594279759200527268078738623930394668942329370914896599940079590507375669702596756439666102748960633787215285415188757440097915087029597806677305991310314133809420614044615359578465969611699945126562704916274528149283184603386570790830494497573257463709010213075286024149615989772056652371147767391049237625724501965727173445141756593203484634107009301826277750779002612230277568017902572368523088345992463146260217553597800183627323574116428303466187123853728583196710745517517535358390707392142847337569520517907785592517088192226248043758847388926042386194109223749016069372127324062038676433519493108834897679875936743283578227831515029271833004625318824853722160041051755203800110174747880289060625994912044287612378359088556065625246018635790500914076191439918598667743937297559655094970714020006618496348625967305473951238868757826364413821456631254354535972707643378303742746504195366925683669474252273786690925881074373957950536867961073627490165000240704238728519647790865820451663465619716223395668776722264188162958730949161467232511411439104654922064720337142462986159828838334648766456731933873743225363023672070688260601987216708838704867314586153630901313495513111642228701713679240756375918406757485802550621390225930614926168099275260457933657233665059918500810800760783452520039166800267236031767353119811507595949084742214031774330195295184873141551680265791895758268696250195805872728809394511284458939711712334163519198881997064641182607001984672164146959109216402440139534348914796507058381160110567423665540425003738328954912183246033347861541003531213135973451967435783624650843915918907173834275229679522001174149998582071145513918745913557852095986167061022492545574120705909462524344111990614935124109843462972392381196792182169294313838491851529627642534803362969065499299101482595350676271526992105965363581364886771627097103730326454063985576045514371607042075088664819647092723929544772484111516471126427516674013320431128243647163056174914074487243575463379929857410388649441301822662353515625");

			for (int i = 0; i < 16494 - 112 + high_bit; i++)
				base = ascii_add(&base, &base);
		}
		

		// Начать считать
		av_t num = av_from_string("0");

		// Но ограничить mantissa_size
		if (mantissa_size > 112)
			mantissa_size = 112;


		uint64_t slice = src->lo;

		for (int i = 0; i < mantissa_size; i++) {
			// Если собираемся просмотреть 65-й бит, то пора переключить кусок
			if (i == 64) slice = src->hi;

			if (slice & 1)
				num = ascii_add(&num, &base);

			base = ascii_add(&base, &base);
			slice = slice >> 1;
		}


		// Финальный разряд в режиме <1.0
		// За исключением числа 0.0 и других крайне маленьких чисел, которые дают exp == 0
		if ((high_bit < 0) && exp)
			num = ascii_add(&num, &base);

		// Спрятать хвостовые нули
		if (exp) while (*num.lsc == '0' && num.lsc > num.msc) {
			num.lsc--;
		}

		print(".");
		print_av(&num);
	}
}

void addition_test(float128_t a, float128_t b) {
	print_float128(&a);
	print(" + ")
	print_float128(&b);
	print(" = ")

	float128_add(&a, &b);

	print_float128(&a);
	print("\n");
}


void addition_1() {
	print(__func__);
	print(": ")

	// Тест: сложение двух одинаковых чисел, переполнение в целую часть
	float128_t a = {0x3FFE400000000000, 0x0000000000000000}; // 0.625
	float128_t b = {0x3FFE400000000000, 0x0000000000000000}; // 0.625
	addition_test(a, b);
}

void addition_2() {
	print(__func__);
	print(": ")

	// Тест: сложение разных чисел, мантисса должна сдвинуться
	float128_t a = {0x400F000380000000, 0x0000000000000000}; // 65539.5
	float128_t b = {0x400A000380000000, 0x0000000000000000}; // 2048.109375
	addition_test(a, b);
}

void addition_3() {
	print(__func__);
	print(": ")

	// Тест: сложение разных чисел, мантисса должна сдвинуться, переполнение в целую часть
	float128_t a = {0x400F000380000000, 0x0000000000000000}; // 2^16 + 3.5
	float128_t b = {0x3FFE400000000000, 0x0000000000000000}; // 0.625
	addition_test(a, b);
}

void addition_4() {
	print(__func__);
	print(": ")

	// Тест: одна из мантисс должна сдвинуться на 64 бита, т.е. полностью мигрирует из hi в lo
	float128_t a = {0x4041678123456789, 0x0000000000000000}; // 1581117267615268.0
	float128_t b = {0x4001234567891234, 0x0000000000000000}; // 4.551111110...
	addition_test(a, b);
}

void addition_5() {
	print(__func__);
	print(": ")

	// Тест: переполнение в lo
	float128_t a = {0x402E000000000000, 0xFFFFFFFFFFFFFFFF}; // 140737488355328.499999999999999999972..
	float128_t b = {0x402E000000000000, 0x0000000000000001}; // 140737488355328.000000000000000000027..
	addition_test(a, b);
}

int main() {
	addition_1();
	addition_2();
	addition_3();
	addition_4();
	addition_5();
}
