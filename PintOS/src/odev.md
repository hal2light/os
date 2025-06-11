5. Proje 4: Dosya Sistemleri

Önceki iki ödevde, dosya sistemi uygulamasının nasıl çalıştığına bakmadan dosya sistemini geniş bir şekilde kullandınız. Bu son ödevde, dosya sistemi uygulamasını geliştireceksiniz. Çalışmalarınız esasen "filesys" dizininde olacak.

Proje 4'ü proje 2 veya proje 3 üzerine inşa edebilirsiniz. Her iki durumda da, proje 2 için gereken tüm işlevsellik filesys teslimatınızda çalışmalıdır. Eğer proje 3 üzerine inşa ediyorsanız, proje 3 işlevselliğinin de çalışması gerekir ve "filesys/Make.vars" dosyasını VM işlevselliğini etkinleştirecek şekilde düzenlemeniz gerekir. VM'yi etkinleştirirseniz %5 ek kredi alabilirsiniz.

 

5.1 Arka Plan

 

5.1.1 Yeni Kod

İşte size muhtemelen yeni olan bazı dosyalar. Bunlar, belirtilen yerler dışında "filesys" dizininde yer alır:

 

"fsutil.c"
    Kernel komut satırından erişilebilen dosya sistemi için basit yardımcı programlar.

     
"filesys.h"
     
"filesys.c"
    Dosya sistemine üst düzey arayüz. Giriş için 3.1.2 Dosya Sistemini Kullanma bölümüne bakınız.

     
"directory.h"
     
"directory.c"
    Dosya adlarını inode'lara dönüştürür. Dizin veri yapısı bir dosya olarak saklanır.

     
"inode.h"
     
"inode.c"
    Bir dosyanın disk üzerindeki veri düzenini temsil eden veri yapısını yönetir.

     
"file.h"
     
"file.c"
    Dosya okuma ve yazmalarını disk sektör okuma ve yazmalarına dönüştürür.

     
"lib/kernel/bitmap.h"
     
"lib/kernel/bitmap.c"
    Bir bitmap veri yapısı ve bu bitmap'i disk dosyalarına okuma ve yazma rutinleri.

Dosya sistemimiz Unix-benzeri bir arayüze sahiptir, bu yüzden creat, open, close, read, write, lseek ve unlink komutları için Unix man sayfalarını okumanız faydalı olabilir. Dosya sistemimiz benzer, ancak tam olarak aynı olmayan çağrılara sahiptir. Bu çağrılar, disk işlemlerine dönüştürülür.

Yukarıdaki kodda tüm temel işlevsellik mevcuttur, böylece dosya sistemi baştan kullanılabilir olacaktır, önceki iki projede gördüğünüz gibi. Ancak, ciddi sınırlamaları vardır ve bunları kaldıracaksınız.

Çoğu çalışmanız "filesys" dizininde olacak, ancak önceki tüm bölümlerle etkileşime girmeye hazırlıklı olmalısınız.

 

5.1.2 Dosya Sistemi Kalıcılığını Test Etme

Artık, Pintos testlerini çalıştırmanın temel sürecine aşina olmalısınız. Gözden geçirme yapmak gerekirse, 1.2.1 Test Etme bölümüne bakabilirsiniz.

Şimdiye kadar, her test sadece bir kez Pintos'u çağırdı. Ancak, bir dosya sisteminin önemli bir amacı, verilerin bir başlatmadan diğerine erişilebilir kalmasını sağlamaktır. Bu nedenle, dosya sistemi projesinin parçası olan testler, Pintos'u ikinci kez çağırır. İkinci çalıştırma, dosya sistemindeki tüm dosya ve dizinleri tek bir dosyada birleştirir, ardından bu dosyayı Pintos dosya sisteminden ana (Unix) dosya sistemine kopyalar.

Değerlendirme betikleri, ikinci çalıştırmada kopyalanan dosyanın içeriğine dayanarak dosya sisteminin doğruluğunu kontrol eder. Bu, dosya sistemi yeterince iyi uygulanana kadar, dosya sistemi projeleri herhangi bir genişletilmiş dosya sistemi testini geçemeyeceği anlamına gelir. tar programı, dosyanın kopyalanmasında kullanılan Pintos kullanıcı programıdır ve oldukça talepkar bir programdır (hem genişletilebilir dosya hem de alt dizin desteği gerektirir), bu nedenle biraz çalışma gerekecektir. Bu aşamaya kadar, make check komutundan dosya sistemi çıkarma hatalarını görmezden gelebilirsiniz.

Bu arada, tahmin etmiş olabileceğiniz gibi, dosya sistemi içeriğini kopyalamak için kullanılan dosya formatı standart Unix "tar" formatıdır. Bunları incelemek için Unix tar programını kullanabilirsiniz. Test t için tar dosyası "t.tar" adıyla kaydedilir.

 

5.2 Uygulama İçin Önerilen Sıra

İşinizi kolaylaştırmak için, bu projedeki bölümleri aşağıdaki sırayla uygulamanızı öneririz:

 

    Buffer önbelleği (bakınız bölüm 5.3.4 Buffer Önbelleği). Buffer önbelleğini uygulayın ve bunu mevcut dosya sistemine entegre edin. Bu noktada, proje 2'deki tüm testlerin (ve eğer proje 3 üzerine inşa ediyorsanız proje 3'ün testlerinin) hala geçmesi gerekir.

     
    Genişletilebilir dosyalar (bakınız bölüm 5.3.2 İndeksli ve Genişletilebilir Dosyalar). Bu adım sonrasında, projeniz dosya büyüme testlerini geçmelidir.

     
    Alt dizinler (bakınız bölüm 5.3.3 Alt Dizinler). Sonrasında, projeniz dizin testlerini geçmelidir.

     
    Kalan çeşitli öğeler.

Genişletilebilir dosyaları ve alt dizinleri paralel olarak uygulayabilirsiniz, eğer geçici olarak yeni dizinlerdeki giriş sayısını sabitlerseniz.

Tüm süreç boyunca senkronizasyonu göz önünde bulundurmalısınız.

 

5.3 Gereksinimler

 

5.3.1 Tasarım Belgesi

Projenizi teslim etmeden önce, sizinle paylaşılan os_odev4.md dosyasını kaynak ağacınıza "pintos/src/filesys/DESIGNDOC" adıyla kopyalamanız ve doldurmanız gerekir. Projeye başlamadan önce tasarım belgesi şablonunu okumanızı öneririz. Bir örnek tasarım belgesi için D. Proje Belgeleri bölümüne bakabilirsiniz.

 

5.3.2 İndeksli ve Genişletilebilir Dosyalar

Temel dosya sistemi, dosyaları tek bir blok olarak tahsis eder ve bu da onu dışsal parçalanmaya karşı savunmasız hale getirir. Yani, n bloklu bir dosya, n blok boş olsa bile tahsis edilemeyebilir. Bu sorunu, disk üzerindeki inode yapısını değiştirerek ortadan kaldırın. Pratikte, bu muhtemelen doğrudan, dolaylı ve çift dolaylı bloklarla bir indeks yapısı kullanmak anlamına gelir. Farklı bir plan seçmekte özgürsünüz, ancak bunun tasarım belgenizde gerekçesini açıklamanız ve dışsal parçalanmaya (bizim sağladığımız blok tabanlı dosya sistemi gibi) karşı duyarlı olmamanız gerekir.

Dosya sistemi bölümünün 8 MB'den büyük olmayacağını varsayabilirsiniz. Dosya sistemi, partition büyüklüğüne kadar olan (meta veriler hariç) dosyaları desteklemelidir. Her inode, bir disk sektörüne kaydedilir, bu da içerebileceği blok işaretçileri sayısını sınırlar. 8 MB dosyaları desteklemek için çift dolaylı blokları uygulamanız gerekecek.

Bir blok tabanlı dosya yalnızca ardında boş alan varsa büyüyebilir, ancak indeksli inode'lar, dosya büyümesini, boş alan bulunduğu sürece mümkün kılar. Dosya büyümesini uygulayın. Temel dosya sisteminde, dosya boyutu dosya oluşturulurken belirtilir. Modern dosya sistemlerinin çoğunda, dosya başlangıçta 0 boyutunda oluşturulur ve ardından dosyanın sonuna yazı yazıldıkça genişletilir. Dosya sisteminizin buna izin vermesi gerekir.

Bir dosyanın boyutu için önceden belirlenmiş bir sınır olmamalıdır, tek kısıtlama, dosyanın dosya sisteminin boyutunu (meta veriler hariç) aşamamasıdır. Bu kısıtlama, şimdi 16 dosyalık başlangıç sınırını aşabilen kök dizini dosyası için de geçerlidir.

Kullanıcı programlarının, mevcut dosya sonu (EOF) sonrasına seek yapmalarına izin verilir. Seek işlemi dosyayı genişletmez. EOF sonrasına yazı yazmak, dosyayı yazılan konum kadar genişletir ve önceki EOF ile yazma başlangıcı arasındaki boşluk sıfırlarla doldurulmalıdır. EOF sonrası başlatılan bir okuma, hiç veri döndürmez.

EOF'dan çok uzaklara yazmak, birçok bloğun tamamen sıfır olmasına neden olabilir. Bazı dosya sistemleri, bu sıfırlanmış bloklar için gerçek veri blokları tahsis eder ve yazar. Diğer dosya sistemleri ise, bu blokları yalnızca açıkça yazıldıklarında tahsis eder. Bu tür dosya sistemleri "sparse files" (seyrek dosyalar) desteği sunar. Dosya sisteminizde her iki tahsis stratejisinden birini benimseyebilirsiniz.

5.3.3 Alt Dizinler

Hiyerarşik bir isim alanı uygulayın. Temel dosya sisteminde, tüm dosyalar tek bir dizinde bulunur. Bunu, dizin girişlerinin dosyalara veya diğer dizinlere işaret etmesine izin verecek şekilde değiştirin.

Dizinlerin, tıpkı diğer dosyalar gibi, orijinal boyutlarının ötesine genişleyebileceğinden emin olun.

Temel dosya sistemi, dosya adlarında 14 karakterlik bir sınırlama uygular. Bu sınırlamayı, bireysel dosya adı bileşenleri için koruyabilir veya isteğinize bağlı olarak genişletebilirsiniz. Ancak, tam yol adlarının 14 karakterden çok daha uzun olmasına izin vermelisiniz.

Her işlem için ayrı bir geçerli dizin (current directory) tutun. Başlangıçta, kök dizini ilk işlem için geçerli dizin olarak ayarlayın. Bir işlem, exec sistem çağrısı ile başka bir işlem başlattığında, çocuk işlem ebeveyninin geçerli dizinini miras alır. Bundan sonra, iki işlemin geçerli dizinleri bağımsızdır, yani bir işlem kendi geçerli dizinini değiştirse bile diğerini etkilemez. (Bu, Unix'te cd komutunun bir kabuk (shell) yerleşik komutu olmasının nedenidir, harici bir program değil.)

Mevcut sistem çağrılarını güncelleyin, böylece dosya adı verilen her yerde, mutlak veya görece bir yol adı kullanılabilir. Dizin ayırıcı karakteri eğik çizgi ("/") olacaktır. Ayrıca, Unix'te olduğu gibi "." ve ".." gibi özel dosya adlarını da desteklemeniz gerekir.

open sistem çağrısını güncelleyin, böylece dizinleri de açabilir. Mevcut sistem çağrılarından yalnızca close, bir dizin için dosya tanımlayıcısını kabul etmelidir.

remove sistem çağrısını güncelleyin, böylece boş dizinleri (kök dizin dışında) normal dosyaların yanı sıra silebilir. Dizinler yalnızca içinde hiç dosya veya alt dizin bulunmuyorsa (sadece "." ve ".." hariç) silinebilir. Bir dizinin, bir işlem tarafından açıkken veya bir işlemin geçerli çalışma dizini olarak kullanılırken silinmesine izin verilip verilmeyeceğine karar verebilirsiniz. Eğer izin veriliyorsa, silinen bir dizinde dosya açma (özellikle "." ve "..") veya yeni dosya oluşturma girişimleri engellenmelidir.

Aşağıdaki yeni sistem çağrılarını uygulayın:

 

Sistem Çağrısı: bool chdir (const char *dir)
    İşlemin geçerli çalışma dizinini, dir olarak değiştirir. Bu yol adı görece veya mutlak olabilir. Başarılıysa true, başarısızsa false döner.

 

Sistem Çağrısı: bool mkdir (const char *dir)
    dir adında bir dizin oluşturur. Bu yol adı görece veya mutlak olabilir. Başarılıysa true, başarısızsa false döner. dir zaten mevcutsa veya dir içindeki herhangi bir dizin adı, sonuncu hariç, mevcut değilse başarısız olur. Yani, mkdir("/a/b/c") ancak "/a/b" zaten mevcutsa ve "/a/b/c" mevcut değilse başarılı olur.

 

Sistem Çağrısı: bool readdir (int fd, char *name)
    Bir dizin girişi okur, fd dosya tanımlayıcısı dizini temsil etmelidir. Başarılıysa, null ile sonlandırılmış dosya adını name değişkenine kaydeder, bu değişkenin READDIR_MAX_LEN + 1 baytlık bir alanı olmalıdır ve true döner. Dizin içinde hiç giriş kalmadıysa, false döner.

    readdir tarafından "." ve ".." döndürülmemelidir.

    Dizin açıkken değişirse, bazı girişlerin hiç okunmaması veya birden fazla kez okunması kabul edilebilir. Aksi takdirde, her dizin girişi yalnızca bir kez okunmalıdır, hangi sırayla olduğu fark etmez.

    READDIR_MAX_LEN, "lib/user/syscall.h" dosyasında tanımlıdır. Eğer dosya sisteminiz, temel dosya sisteminden daha uzun dosya adlarını destekliyorsa, bu değeri varsayılan 14'ten arttırmalısınız.

 

Sistem Çağrısı: bool isdir (int fd)
    Başarılıysa true döner, eğer fd bir dizini temsil ediyorsa; false döner, eğer bir normal dosya temsil ediyorsa.

 

Sistem Çağrısı: int inumber (int fd)
    fd ile ilişkilendirilmiş inode'un inode numarasını döner, bu inode bir normal dosya veya dizin olabilir.

    Bir inode numarası, bir dosya veya dizini kalıcı olarak tanımlar. Dosyanın varlığı süresince benzersizdir. Pintos'ta, inode'un sektör numarası, inode numarası olarak kullanılabilir.

Biz, yukarıdaki sistem çağrıları uygulandıktan sonra, oldukça basit olan ls ve mkdir kullanıcı programlarını sağladık. Ayrıca, pek de basit olmayan pwd programını da sağladık. shell programı ise cd komutunu dahili olarak uygular.

pintos "extract" ve "append" komutları artık tam yol adlarını kabul etmelidir, bunun için yol içinde kullanılan dizinlerin zaten oluşturulmuş olması gerektiğini varsayarsak. Bu, sizin için önemli bir ekstra çaba gerektirmemelidir.

 

5.3.4 Arabellek (Buffer) Önbelleği

Dosya sistemini, dosya bloklarının bir önbelleğini tutacak şekilde değiştirin. Bir blok okuma veya yazma isteği yapıldığında, önbellekte olup olmadığını kontrol edin ve eğer varsa, diske gitmeden önbellekten veriyi kullanın. Aksi takdirde, bloğu diske alıp önbelleğe kopyalayın, gerekirse daha eski bir girişi çıkartarak. Önbellek boyutunuz 64 sektörden fazla olmamalıdır.

En azından "clock" algoritması kadar iyi bir önbellek değiştirme algoritması uygulamalısınız. Meta verilerin, verilere kıyasla genellikle daha değerli olduğunu dikkate almanızı tavsiye ederiz. Disk erişim sayısı ile ölçülen en iyi performansı elde etmek için, erişilen, kirli ve diğer bilgilerin hangi kombinasyonunun en iyi sonuçları verdiğini görmek için denemeler yapın.

Özgür harita (free map) verisini bellek içinde kalıcı olarak tutmak isterseniz, buna izin verilir. Bu, önbellek boyutuna dahil edilmek zorunda değildir.

Sağlanan inode kodu, diskin sektör bazlı arayüzünü, sistem çağrısı arayüzünün bayt bazlı arayüzüne çevirmek için malloc() ile tahsis edilmiş bir "bounce buffer" kullanır. Bu "bounce buffer"ları kaldırmalısınız. Bunun yerine, verileri doğrudan önbellek içindeki sektörlere kopyalayın.

Önbelleğiniz yazma-ardında (write-behind) olmalıdır, yani kirli blokları hemen diske yazmak yerine önbellekte tutun. Kirli blokları, diske çıkartıldıklarında yazın. Yazma-ardında yöntemi, dosya sisteminizi çökme durumlarına karşı daha kırılgan hale getirdiği için, ayrıca tüm kirli, önbellekli blokları periyodik olarak diske yazmalısınız. Önbellek ayrıca filesys_done() içinde diske yazılmalı, böylece Pintos'un durdurulması önbelleği temizler.

Eğer ilk projeden timer_sleep() çalışıyorsa, yazma-ardında mükemmel bir uygulamadır. Aksi takdirde, daha az genel bir sistem uygulayabilirsiniz, ancak bunun bekleme yapmadığından emin olmalısınız.

Ayrıca okuma-önceden (read-ahead) uygulamalısınız, yani bir dosyanın bir bloğu okunduğunda, bir sonraki bloğu otomatik olarak önbelleğe alın. Okuma-önceden yalnızca asenkron olarak yapılırsa gerçekten yararlı olur. Bu, bir işlem, dosya bloğu 1'i okuma isteği gönderirse, blok 1 okunduğunda engellenmesi gerektiği, ancak okuma tamamlandığında kontrolün hemen işleme geri verilmesi gerektiği anlamına gelir. Dosya bloğu 2'ye yönelik okuma-önceden isteği asenkron olarak, arka planda işlenmelidir.

Önbelleği tasarımınıza erken entegre etmenizi tavsiye ederiz. Geçmişte, birçok grup tasarım sürecinin sonlarına doğru önbelleği tasarımlarına eklemeye çalıştı. Bu çok zordur. Bu gruplar genellikle çoğu veya tüm testlerden başarısız olmuş projeler teslim etmiştir.

 

5.3.5 Senkronizasyon

Sağlanan dosya sistemi, dış senkronizasyon gerektirir, yani çağıranlar dosya sistemi kodunda yalnızca bir iş parçacığının aynı anda çalışmasını sağlamalıdır. Gönderiminiz, dış senkronizasyon gerektirmeyen daha ince taneli bir senkronizasyon stratejisini benimsemelidir. Mümkün olduğunca, bağımsız varlıklar üzerindeki işlemler bağımsız olmalıdır, böylece birbirlerini beklemek zorunda kalmazlar.

Farklı önbellek blokları üzerindeki işlemler bağımsız olmalıdır. Özellikle, belirli bir blok üzerinde I/O gerektiğinde, I/O'ya ihtiyaç duymayan diğer bloklardaki işlemler, I/O tamamlanmasını beklemek zorunda kalmadan devam etmelidir.

Birden fazla işlem, aynı dosyaya aynı anda erişebilmelidir. Bir dosyanın birden fazla okuması, birbirini beklemeden tamamlanabilmelidir. Bir dosya yazılırken, dosya uzatılmadığı sürece, birden fazla işlem aynı dosyaya aynı anda yazabilmelidir. Bir işlem tarafından okunan dosya, başka bir işlem tarafından yazılırken, yazmanın tamamlanıp tamamlanmadığını gösterebilir. (Ancak, write sistem çağrısı geri döndükten sonra, tüm sonraki okuyucular değişikliği görmelidir.) Benzer şekilde, iki işlem aynı dosyanın aynı kısmına aynı anda yazarsa, verileri birbirine karışmış olabilir.

Öte yandan, bir dosyayı uzatmak ve yeni bölüme veri yazmak atomik olmalıdır. A ve B işlemlerinin her ikisi de verilen bir dosyayı açık tutuyor ve her ikisi de dosyanın sonuna yerleşmişse, A okurken ve B dosyaya yazarken, A B'nin yazdığı her şeyi, bir kısmını veya hiçbirini okuyabilir. Ancak A, B'nin yazdığı veriden başka bir şey okuyamaz; örneğin, B'nin verisi tümüyle sıfır olmayan baytlardan oluşuyorsa, A'nın sıfır görmesine izin verilmez.

Farklı dizinler üzerindeki işlemler aynı anda gerçekleşebilir. Aynı dizin üzerindeki işlemler ise birbirini bekleyebilir.

Unutmayın ki yalnızca birden fazla iş parçacığı tarafından paylaşılan veriler senkronize edilmelidir. Temel dosya sisteminde, struct file ve struct dir yalnızca bir iş parçacığı tarafından erişilir.

 

5.4 SSS (Sıkça Sorulan Sorular)

 

Ne kadar kod yazmam gerekecek?

    İşte, diffstat programı tarafından üretilen referans çözümümüzün bir özeti. Son satır, eklenen ve silinen toplam satır sayısını verir; değiştirilen bir satır, hem ekleme hem de silme olarak sayılır.

    Bu özet, Pintos temel koduna göre yapılmıştır, ancak proje 4 için referans çözümü, proje 3 için referans çözümüne dayanmaktadır. Dolayısıyla, referans çözümü sanal bellek etkinleştirilmiş olarak çalışmaktadır. Proje 3'ün özeti için bölüm 4.4 SSS'yi inceleyin.

    Referans çözümü yalnızca bir olasılık çözümünü temsil eder. Birçok başka çözüm de mümkündür ve bunların çoğu, referans çözümünden önemli ölçüde farklıdır. Bazı mükemmel çözümler, referans çözümü tarafından değiştirilen tüm dosyaları değiştirmeyebilir ve bazıları, referans çözümü tarafından değiştirilmemiş dosyaları değiştirebilir.

     
      	

     Makefile.build       |    5 
     devices/timer.c      |   42 ++
     filesys/Make.vars    |    6 
     filesys/cache.c      |  473 +++++++++++++++++++++++++
     filesys/cache.h      |   23 +
     filesys/directory.c  |   99 ++++-
     filesys/directory.h  |    3 
     filesys/file.c       |    4 
     filesys/filesys.c    |  194 +++++++++-
     filesys/filesys.h    |    5 
     filesys/free-map.c   |   45 +-
     filesys/free-map.h   |    4 
     filesys/fsutil.c     |    8 
     filesys/inode.c      |  444 ++++++++++++++++++-----
     filesys/inode.h      |   11 
     threads/init.c       |    5 
     threads/interrupt.c  |    2 
     threads/thread.c     |   32 +
     threads/thread.h     |   38 +-
     userprog/exception.c |   12 
     userprog/pagedir.c   |   10 
     userprog/process.c   |  332 +++++++++++++----
     userprog/syscall.c   |  582 ++++++++++++++++++++++++++++++-
     userprog/syscall.h   |    1 
     vm/frame.c           |  161 ++++++++
     vm/frame.h           |   23 +
     vm/page.c            |  297 +++++++++++++++
     vm/page.h            |   50 ++
     vm/swap.c            |   85 ++++
     vm/swap.h            |   11 
     30 dosya değiştirildi, 2721 ekleme(+), 286 silme(-)

     
BLOCK_SECTOR_SIZE değişebilir mi?

    Hayır, BLOCK_SECTOR_SIZE sabittir ve 512'dir. IDE diskler için bu değer, donanımın sabit bir özelliğidir. Diğer diskler, 512 baytlık bir sektöre sahip olmayabilir, ancak basitlik adına Pintos yalnızca 512 baytlık sektörlere sahip olanları destekler.

 

5.4.1 İndeksli Dosyalar SSS

 

Desteklememiz gereken en büyük dosya boyutu nedir?

    Oluşturduğumuz dosya sistemi bölümü 8 MB veya daha küçük olacak. Ancak, bireysel dosyalar, meta verileri barındırmak için bölümden küçük olmalıdır. Inode organizasyonunuzu belirlerken bunu dikkate almanız gerekecek.

 

5.4.2 Alt Dizinler SSS

 

Bir dosya adı olan "a//b" nasıl yorumlanmalıdır?

    Birden fazla ardışık eğik çizgi, bir tek eğik çizgiyle eşdeğerdir, bu nedenle bu dosya adı "a/b" ile aynıdır.

     
Peki ya "/../x" gibi bir dosya adı?

    Kök dizini kendi ana dizini olduğundan, bu "/x" ile eşdeğerdir.

     
Sonu "/" ile biten bir dosya adı nasıl işlenmelidir?

    Çoğu Unix sistemi, dizin için adın sonuna eğik çizgi koyulmasına izin verir ve eğik çizgiyle biten diğer adları reddeder. Biz de bu davranışa izin vereceğiz, ayrıca sonu eğik çizgiyle biten bir adı reddedeceğiz.

 

5.4.3 Tampon Bellek SSS

 

struct inode_disk yapısını struct inode içinde tutabilir miyiz?

    64 blok sınırının amacı, önbelleğe alınan dosya sistemi verisinin miktarını sınırlamaktır. Eğer bir disk verisi bloğunu—ister dosya verisi ister meta veri—çekirdek belleğinde herhangi bir yerde tutarsanız, bu 64 blok sınırına dahil edilmelidir. Aynı kural, disk verisi bloğuna "benzer" olan herhangi bir şey için de geçerlidir, örneğin, struct inode_disk ama length veya sector_cnt üye alanları olmadan.

    Bu, inode uygulamasının şu anki inode'uyla disk üzerindeki karşılık gelen inode'lara nasıl eriştiğini değiştirmeniz gerektiği anlamına gelir, çünkü şu anda sadece bir struct inode_disk nesnesini struct inode içine gömüp, yaratıldığında karşılık gelen sektörü diskinizden okur. İnode'ların ekstra kopyalarını tutmak, önbelleğiniz üzerindeki 64 blok sınırlamasını ihlal eder.

    Inode verilerine bir işaretçi tutabilirsiniz, ancak bunu yaparsanız, işletim sisteminizin aynı anda yalnızca 64 dosyayı açık tutmasını sınırlamadığından emin olmalısınız. Ayrıca, inode'ları bulmanıza yardımcı olacak başka bilgiler de tutabilirsiniz. Benzer şekilde, 64 önbellek girişinizin her birinde bazı meta verileri saklayabilirsiniz.

    Özgür harita (free map) gibi bir şeyi bellekte kalıcı olarak tutabilirsiniz. Bu, önbellek boyutuna dahil edilmek zorunda değildir.

    byte_to_sector() fonksiyonu, "filesys/inode.c" dosyasında, struct inode_disk yapısını doğrudan kullanır, o sektörü nerede olursa olsun önce okumadan. Bu artık çalışmayacaktır. inode_byte_to_sector() fonksiyonunu, struct inode_disk yapısını önbellekten alacak şekilde değiştirmeniz gerekecek.
