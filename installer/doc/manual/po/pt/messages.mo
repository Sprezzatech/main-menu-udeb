��    D      <  a   \      �  �   �  &   s  �  �    M	    f
  �   k  �  �  -  �  �    �   �  �   d  �   ^  �    �   �  �   �  |   �  �       �  u   �  �   V  V  C  �   �  d   s  �   �  �   �     [  +   j  w   �  �        �   B   "  �  H"  :  $  s   I%  �  �%  T  A'    �(    �*  �   �,     {-  �  �-    �/  �   �0  -  =1  M  k2  ^  �3  5  5  �  N6    �7  �  {9     8<  �  9>  �  @  ]  �A  �   9C  e  �C  �   bE  �  bF  �  HH     �J     �J     �J     K     K  �   *K  "   )L  �   LL  5  �L  �   N  4   �N  �  �N  4  �Q  C  S  �   ST    �T  C  �V  �  <X  �   Z    [  �   4\  �  �\  
  �_  I  �`  |   %b  �   �b  K  �c  �   �d    _e  �  uf  �   
h  e   i  �   gi  �   Jj     k  ,   "k  �   Ok  �   �k     �l  B   �m  �  n  W  �o  }   Pq  �  �q  �  �s    8u  *  ;w  �   fy     0z    Nz  6  f|  �   �}  :  ,~  i  g  h  р  a  :�  �  ��  �  i�  �  �  #  ��  �  �  �  �  �  �  �   j�  �  K�  8  �  I  +�  �  u�     �     )�     <�     V�     g�     {�  )   ��  �   ƛ            4      "   	              A       $   ,              :   B                       9       !      7       &              (   ?         
                  '   /   )   1   >   <       @                   3             D      5   +   .      ;          *   8      #              2              6      C                                     0   -   %   =    <ulink url="&url-debian-jr;">Debian Jr.</ulink> is an internal project, aimed at making sure Debian has something to offer to our youngest users. About Copyrights and Software Licenses An operating system consists of various fundamental programs which are needed by your computer so that it can communicate and receive instructions from users; read and write data to hard disks, tapes, and printers; control the use of memory; and run other software. The most important part of an operating system is the kernel. In a GNU/Linux system, Linux is the kernel component. The rest of the system consists of other programs, many of which were written by or for the GNU Project. Because the Linux kernel alone does not form a working operating system, we prefer to use the term <quote>GNU/Linux</quote> to refer to systems that many people casually refer to as <quote>Linux</quote>. Any help, suggestions, and especially, patches, are greatly appreciated. Working versions of this document can be found at <ulink url="&url-install-manual;"></ulink>. There you will find a list of all the different architectures and languages for which this document is available. Backup your system, perform any necessary planning and hardware configuration prior to installing Debian, in <xref linkend="preparing"/>. If you are preparing a multi-boot system, you may need to create partition-able space on your hard disk for Debian to use. Boot into your newly installed base system and run through some additional configuration tasks, from <xref linkend="boot-new"/>. Calling software <emphasis>free</emphasis> doesn't mean that the software isn't copyrighted, and it doesn't mean that CDs containing that software must be distributed at no charge. Free software, in part, means that the licenses of individual programs do not require you to pay for the privilege of distributing or using those programs. Free software also means that not only may anyone extend, adapt, and modify the software, but that they may distribute the results of their work as well. Debian Developers are involved in a variety of activities, including <ulink url="&url-debian-home;">Web</ulink> and <ulink url="&url-debian-ftp;">FTP</ulink> site administration, graphic design, legal analysis of software licenses, writing documentation, and, of course, maintaining software packages. Debian GNU/Hurd is a Debian GNU system that replaces the Linux monolithic kernel with the GNU Hurd &mdash; a set of servers running on top of the GNU Mach microkernel. The Hurd is still unfinished, and is unsuitable for day-to-day use, but work is continuing. The Hurd is currently only being developed for the i386 architecture, although ports to other architectures will be made once the system becomes more stable. Debian can be upgraded after installation very easily. The installation procedure will help set up the system so that you can make those upgrades once installation is complete, if need be. Debian continues to be a leader in Linux development. Its development process is an example of just how well the Open Source development model can work &mdash; even for very complex tasks such as building and maintaining a complete operating system. Debian developers are also involved in a number of other projects; some specific to Debian, others involving some or all of the Linux community. Some examples include: Debian is an all-volunteer organization dedicated to developing free software and promoting the ideals of the Free Software Foundation. The Debian Project began in 1993, when Ian Murdock issued an open invitation to software developers to contribute to a complete and coherent software distribution based on the relatively new Linux kernel. That relatively small band of dedicated enthusiasts, originally funded by the <ulink url="&url-fsf-intro;">Free Software Foundation</ulink> and influenced by the <ulink url="&url-gnu-intro;">GNU</ulink> philosophy, has grown over the years into an organization of around &num-of-debian-developers; <firstterm>Debian Developers</firstterm>. Debian is especially popular among advanced users because of its technical excellence and its deep commitment to the needs and expectations of the Linux community. Debian also introduced many features to Linux that are now commonplace. Debian's attention to detail allows us to produce a high-quality, stable, and scalable distribution. Installations can be easily configured to serve many roles, from stripped-down firewalls to desktop scientific workstations to high-end network servers. Determine whether your hardware meets the requirements for using the installation system, in <xref linkend="hardware-req"/>. Development of what later became GNU/Linux began in 1984, when the <ulink url="http://www.gnu.org/">Free Software Foundation</ulink> began development of a free Unix-like operating system called GNU. Expert users may also find interesting reference information in this document, including minimum installation sizes, details about the hardware supported by the Debian installation system, and so on. We encourage expert users to jump around in the document. Finally, information about this document and how to contribute to it may be found in <xref linkend="administrivia"/>. For example, Debian was the first Linux distribution to include a package management system for easy installation and removal of software. It was also the first Linux distribution that could be upgraded without requiring reinstallation. For information on how to download &debian; from the Internet or from whom official Debian CDs can be purchased, see the <ulink url="&url-debian-distrib;">distribution web page</ulink>. The <ulink url="&url-debian-mirrors;">list of Debian mirrors</ulink> contains a full set of official Debian mirrors, so you can easily find the nearest one. For information on how to locate, unpack, and build binaries from Debian source packages, see the <ulink url="&url-debian-faq;">Debian FAQ</ulink>, under <quote>Basics of the Debian Package Management System</quote>. For more general information about Debian, see the <ulink url="&url-debian-faq;">Debian FAQ</ulink>. For more information about licenses and how Debian determines whether software is free enough to be included in the main distribution, see the <ulink url="&url-dfsg;">Debian Free Software Guidelines</ulink>. For more information, see the <ulink url="http://www.debian.org/ports/hurd/"> Debian GNU/Hurd ports page</ulink> and the <email>debian-hurd@lists.debian.org</email> mailing list. Getting Debian Getting the Newest Version of This Document In <xref linkend="install-methods"/>, you will obtain the necessary installation files for your method of installation. In general, this manual is arranged in a linear fashion, walking you through the installation process from start to finish. Here are the steps in installing &debian;, and the sections of this document which correlate with each step: In the interest of communicating our philosophy and attracting developers who believe in the principles that Debian stands for, the Debian Project has published a number of documents that outline our values and serve as guides to what it means to be a Debian Developer: Install additional software in <xref linkend="install-packages"/>. Linus Torvalds continues to coordinate the work of several hundred developers with the help of a few trusty deputies. An excellent weekly summary of discussions on the <userinput>linux-kernel</userinput> mailing list is <ulink url="&url-kernel-traffic;">Kernel Traffic</ulink>. More information about the <userinput>linux-kernel</userinput> mailing list can be found on the <ulink url="&url-linux-kernel-list-faq;">linux-kernel mailing list FAQ</ulink>. Linux is also less likely to crash, better able to run more than one program at the same time, and more secure than many operating systems. With these advantages, Linux is the fastest growing operating system in the server market. More recently, Linux has begun to be popular among home and business users as well. Linux is an operating system: a series of programs that let you interact with your computer and run other programs. Linux is modelled on the Unix operating system. From the start, Linux was designed to be a multi-tasking, multi-user system. These facts are enough to make Linux different from other well-known operating systems. However, Linux is even more different than you might imagine. In contrast to other operating systems, nobody owns Linux. Much of its development is done by unpaid volunteers. Linux users have immense freedom of choice in their software. For example, Linux users can choose from a dozen different command line shells and several graphical desktops. This selection is often bewildering to users of other operating systems, who are not used to thinking of the command line or desktop as something that they can change. Many of the programs in the system are licensed under the <emphasis>GNU</emphasis> <emphasis>General Public License</emphasis>, often simply referred to as <quote>the GPL</quote>. The GPL requires you to make the <emphasis>source code</emphasis> of the programs available whenever you distribute a binary copy of the program; that provision of the license ensures that any user will be able to modify the software. Because of this provision, the source code for all such programs is available in the Debian system. Note that the Debian project, as a pragmatic concession to its users, does make some packages available that do not meet our criteria for being free. These packages are not part of the official distribution, however, and are only available from the <userinput>contrib</userinput> or <userinput>non-free</userinput> areas of Debian mirrors or on third-party CD-ROMs; see the <ulink url="&url-debian-faq;">Debian FAQ</ulink>, under <quote>The Debian FTP archives</quote>, for more information about the layout and contents of the archives. Once you've got your system installed, you can read <xref linkend="post-install"/>. That chapter explains where to look to find more information about Unix and Debian, and how to replace your kernel. Organization of This Document Perform the actual installation according to <xref linkend="d-i-intro"/>. This involves choosing your language, configuring peripheral driver modules, configuring your network connection, so that remaining installation files can be obtained directly from a Debian server (if you are not installing from a CD), partitioning your hard drives and installation of minimal working system. (Some background about setting up the partitions for your Debian system is explained in <xref linkend="partitioning"/>.) Source is also available publicly; look in <xref linkend="administrivia"/> for more information concerning how to contribute. We welcome suggestions, comments, patches, and bug reports (use the package &d-i-manual; for bugs, but check first to see if the problem is already reported). The <ulink url="&url-debian-policy;">Debian Policy Manual</ulink> is an extensive specification of the Debian Project's standards of quality. The <ulink url="&url-dfsg;">Debian Free Software Guidelines</ulink> are a clear and concise statement of Debian's criteria for free software. The DFSG is a very influential document in the Free Software Movement, and was the foundation of the <ulink url="&url-osd;">The Open Source Definition</ulink>. The <ulink url="&url-fhs-home;">Filesystem Hierarchy Standard</ulink> (FHS) is an effort to standardize the layout of the Linux file system. The FHS will allow software developers to concentrate their efforts on designing programs, without having to worry about how the package will be installed in different GNU/Linux distributions. The <ulink url="&url-kernel-org;">Linux kernel</ulink> first appeared in 1991, when a Finnish computing science student named Linus Torvalds announced an early version of a replacement kernel for Minix to the Usenet newsgroup <userinput>comp.os.minix</userinput>. See Linux International's <ulink url="&url-linux-history;">Linux History Page</ulink>. The <ulink url="&url-lsb-org;">Linux Standard Base</ulink> (LSB) is a project aimed at standardizing the basic GNU/Linux system, which will enable third-party software and hardware developers to easily design programs and device drivers for Linux-in-general, rather than for a specific GNU/Linux distribution. The <ulink url="&url-social-contract;">Debian Social Contract</ulink> is a statement of Debian's commitments to the Free Software Community. Anyone who agrees to abide to the Social Contract may become a <ulink url="&url-new-maintainer;">maintainer</ulink>. Any maintainer can introduce new software into Debian &mdash; provided that the software meets our criteria for being free, and the package follows our quality standards. The GNU Project has developed a comprehensive set of free software tools for use with Unix&trade; and Unix-like operating systems such as Linux. These tools enable users to perform tasks ranging from the mundane (such as copying or removing files from the system) to the arcane (such as writing and compiling programs or doing sophisticated editing in a variety of document formats). The combination of Debian's philosophy and methodology and the GNU tools, the Linux kernel, and other important free software, form a unique software distribution called &debian;. This distribution is made up of a large number of software <emphasis>packages</emphasis>. Each package in the distribution contains executables, scripts, documentation, and configuration information, and has a <emphasis>maintainer</emphasis> who is primarily responsible for keeping the package up-to-date, tracking bug reports, and communicating with the upstream author(s) of the packaged software. Our extremely large user base, combined with our bug tracking system ensures that problems are found and fixed quickly. The feature that most distinguishes Debian from other Linux distributions is its package management system. These tools give the administrator of a Debian system complete control over the packages installed on that system, including the ability to install a single package or automatically update the entire operating system. Individual packages can also be protected from being updated. You can even tell the package management system about software you have compiled yourself and what dependencies it fulfills. The most important legal notice is that this software comes with <emphasis>no warranties</emphasis>. The programmers who have created this software have done so for the benefit of the community. No guarantee is made as to the suitability of the software for any given purpose. However, since the software is free, you are empowered to modify that software to suit your needs &mdash; and to enjoy the benefits of the changes made by others who have extended the software in this way. The primary, and best, method of getting support for your &debian; system and communicating with Debian Developers is through the many mailing lists maintained by the Debian Project (there are more than &num-of-debian-maillists; at this writing). The easiest way to subscribe to one or more of these lists is visit <ulink url="&url-debian-lists-subscribe;"> Debian's mailing list subscription page</ulink> and fill out the form you'll find there. There are several other forms of copyright statements and software licenses used on the programs in Debian. You can find the copyrights and licenses for every package installed on your system by looking in the file <filename>/usr/share/doc/<replaceable>package-name</replaceable>/copyright </filename> once you've installed a package on your system. This chapter provides an overview of the Debian Project and &debian;. If you already know about the Debian Project's history and the &debian; distribution, feel free to skip to the next chapter. This document is constantly being revised. Be sure to check the <ulink url="&url-release-area;"> Debian &release; pages</ulink> for any last-minute information about the &release; release of the &debian; system. Updated versions of this installation manual are also available from the <ulink url="&url-install-manual;">official Install Manual pages</ulink>. This document is meant to serve as a manual for first-time Debian users. It tries to make as few assumptions as possible about your level of expertise. However, we do assume that you have a general understanding of how the hardware in your computer works. To protect your system against <quote>Trojan horses</quote> and other malevolent software, Debian's servers verify that uploaded packages come from their registered Debian maintainers. Debian packagers also take great care to configure their packages in a secure manner. When security problems in shipped packages do appear, fixes are usually available very quickly. With Debian's simple update options, security fixes can be downloaded and installed automatically across the Internet. We're sure that you've read some of the licenses that come with most commercial software &mdash; they usually say that you can only use one copy of the software on a single computer. This system's license isn't like that at all. We encourage you to put a copy of on every computer in your school or place of business. Lend your installation media to your friends and help them install it on their computers! You can even make thousands of copies and <emphasis>sell</emphasis> them &mdash; albeit with a few restrictions. Your freedom to install and use the system comes directly from Debian being based on <emphasis>free software</emphasis>. Welcome to Debian What is &debian;? What is Debian GNU/Hurd? What is Debian? What is GNU/Linux? While many groups and individuals have contributed to Linux, the largest single contributor is still the Free Software Foundation, which created not only most of the tools used in Linux, but also the philosophy and the community that made Linux possible. Your Documentation Help is Welcome describes booting into the installation system. This chapter also discusses troubleshooting procedures in case you have problems with this step. Project-Id-Version: welcome 18673
POT-Creation-Date: 2001-02-09 01:25+0100
PO-Revision-Date: 2005-03-13 19:41+0000
Last-Translator: Miguel Figueiredo <elmig@debianpt.org>
Language-Team: Portuguese <traduz@debianpt.org>
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
 <ulink url="&url-debian-jr;">Debian Jr.</ulink> é um projecto interno, que se destina a assegurar que Debian tem algo a oferecer aos nossos utilizadores mais novos. Acerca de Direitos de Cópia e Licenças de Software O seu sistema operativo consiste em vários programas fundamentais que são necessários ao seu computador de modo a que possa comunicar e receber instruções dos utilizadores; ler e escrever dados em discos rígidos, tapes, e impressoras; controlar a utilização da memória; e correr outro software. A parte mais importante de um sistema operativo é o kernel. Num sistema GNU/Linux, Linux é o componente do kernel. O resto do sistema consiste noutros programas, muitos dos quais escritos por ou para o GNU Project. Devido ao kernel sozinho não formar um sistema operativo utilizável, nós preferimos utilizar o termo <quote>GNU/Linux</quote> para nos referirmos aos sistemas a que muitas pessoas vulgarmente chamam de <quote>Linux</quote>. Qualquer ajuda, sugestões, e especialmente, correcções, são muito apreciadas. Versões de trabalho deste documento podem ser encontradas em <ulink url="&url-install-manual;"></ulink>. Lá você irá encontrar uma lista de diferentes arquitecturas e idiomas para os quais está disponível este documento. Faça cópias de segurança do seu sistema, execute o planeamento e configuração de hardware necessário antes de instalar Debian, em <xref linkend="preparing"/>. Se você estiver a preparar um sistema multi-boot, você pode necessitar de criar o espaço particionável no seu disco rígido para ser utilizado com Debian. Iniciar no seu sistema base acabado de instalar e correr uma série de tarefas de configuração adicionais, a partir de <xref linkend="boot-new"/>. Chamar ao software <emphasis>livre</emphasis> não significa que o software não tem direitos de cópia, e não significa que os CDs que contêm o software tenham de ser distribuídos sem encargos. Software livre, em parte, significa que as licenças dos programas individuais não necessitam que você pague pelo previlégio de distribuir e correr esses programas. Software livre também significa que qualquer um pode extender, adaptar, e modificar o software, mas eles podem também distribuir os resultados do seu trabalho. Os Debian Developers estão involvidos numa série de actividades, incluíndo a administração do site <ulink url="&url-debian-home;">Web</ulink> e do arquivo <ulink url="&url-debian-ftp;">FTP</ulink>, design gráfico, análise legal de licenças de software, escrever documentação, e, claro, manter pacotes de software. Debian GNU/Hurd é um sistema Debian GNU que substitui o kernel monolítico Linux pelo GNU Hurd &mdash; um conjunto de servidores que correm no topo do microkernel do GNU Mach. O Hurd ainda não está terminado e não é por isso adequado para o uso do dia-a-dia, mas o trabalho vai continuando. Actualmente, o Hurd está apenas a ser desenvolvido para a arquitectura i386, no entanto a conversão para outras arquitecturas será feita assim que o sistema se tornar mais estável. Debian pode ser facilmente actualizado após a sua instalação. O próprio procedimento de instalação vai ajudar a configurar o sistema para que, caso seja necessário, possam ser feitas essas mesmas actualizações após a instalação estar completa. Debian continua a ser líder no desenvolvimento de Linux. O seu processo de desenvolvimento é um exemplo de como pode o modelo de desenvolvimento Open Source funcionar bem e &mdash; mesmo para tarefas muito complexas tais como construír e manter um sistema operativo completo. Os Debian Developers também estão envolvidos num número de outros projectos; alguns específicos de Debian, outros envolvendo alguma ou toda a comunidade Linux. Alguns exemplos incluem: Debian é uma organização exclusiva de voluntários dedicada ao desenvolvimento de software livre e a promover os ideais da Free Software Foundation. O Debian Project começou em 1993, quando Ian Murdock lançou um convite aberto a programadores de software para contribuirem para uma distribuição de software completa e coerente baseada no relativamente novo kernel Linux. Esse relativamente pequeno grupo de entusiastas dedicados, originalmente com fundos da <ulink url="&url-fsf-intro;">Free Software Foundation</ulink> e influenciados pela filosofia <ulink url="&url-gnu-intro;">GNU</ulink>, cresceu com o passar dos anos para uma organização com cerca de &num-of-debian-developers; <firstterm>Debian Developers</firstterm>. Debian é especialmente popular entre utilizadores avançados devido à sua excelência técnica e ao seu profundo compromisso com as necessidades e expectativas da comunidade Linux. Debian também introduziu muitas funcionalidade a Linux que agora são lugar-comum. A atenção de Debian para os detalhes permite-nos produzir uma distribuição de alta qualidade, estável e escalável. As instalações podem ser facilmente configuradas para servirem vários papéis, desde firewalls dedicadas a ambientes de trabalho de estações de trabalho ciêntífico a servidores de rede de elevada gama. Determine se o seu hardware cumpre os requisitos para utilizar o sistema de instalação, em <xref linkend="hardware-req"/>. O desenvolvimento do que mais tarde se tornou GNU/Linux começou em 1984, quando a <ulink url="http://www.gnu.org/">Free Software Foundation</ulink> iniciou o desenvolvimento de um sistema operativo livre, ao estilo Unix, chamado GNU. Os utilizadores avançados podem também encontrar uma interessante referência de informação neste documento, incluindo tamanhos minimos de instalação, detalhes acerca do hardware suportado pelo sistema de instalação Debian, e etc. Nós encorajamos os nossos utilizadores avançados a dar uma vista de olhos neste documento. Finalmente, informação acerca deste documento e de como contribuir para ele pode ser encontrada em <xref linkend="administrivia"/>. Por exemplo, Debian foi a primeira distribuição a incluir um sistema de gestão de pacotes para fácil instalação e remoção de software. Foi também a primeira distribuição de Linux a poder ser substituída por uma versão mais recente sem necessitar de reinstalação. Para obter informação acerca de como fazer download de &debian; através da Internet ou ainda para saber onde pode comprar os CDs oficiais Debian, veja <ulink url="&url-debian-distrib;">a página da distribuição</ulink>. A <ulink url="&url-debian-mirrors;">lista de mirrors Debian</ulink> contém a lista completa dos mirrors oficiais Debian para que possa facilmente encontrar o mais próximo de si. Para informação acerca de como encontrar, descompactar, e construír binários dos pacotes de código fonte Debian, veja o <ulink url="&url-debian-faq;">Debian FAQ</ulink>, acerca de <quote>Bases do Sistema de Gestão de Pacotes Debian</quote>. Para mais informações gerais sobre Debian, veja o <ulink url="&url-debian-faq;">Debian FAQ</ulink>. Para mais informação acerca de licenças e de como Debian determina se o software é suficientemente livre para ser incluído na distribuição principal, veja <ulink url="&url-dfsg;">Debian Free Software Guidelines</ulink>. Para mais informações, veja a <ulink url="http://www.debian.org/ports/hurd/">Página do port Debian GNU/Hurd</ulink> e <email>debian-hurd@lists.debian.org</email> que é a mailing list correspondente. Obter Debian Obter a Versão Mais Recente Deste Documento Em <xref linkend="install-methods"/>, você obterá os ficheiros necessários para a instalação para o seu método da instalação. Genericamente, este manual está disposto numa forma linear, acompanhando-o através do processo de instalação desde o início até ao fim. Aqui estão as etapas da instalação de &debian;, e as secções deste documento que combinam com cada etapa: No interesse de comunicar a nossa filosofia e atraír developers que acreditem nos príncipios que Debian defende, o Debian Project publicou uma série de documentos que sublinham os nossos valores e servem de guia ao que significa ser um Debian Developer: Instalar software adicional em <xref linkend="install-packages"/>. Linus Torvalds continua a coordenar o trabalho de várias centenas de programadores com a ajuda de alguns ajudantes de confiança. Um excelente sumário semanal das discussões na mailing list <userinput>linux-kernel</userinput> encontra-se em <ulink url="&url-kernel-traffic;">Kernel Traffic</ulink>. Mais informação acerca da mailing list <userinput>linux-kernel</userinput> pode ser encontrada no <ulink url="&url-linux-kernel-list-faq;">linux-kernel mailing list FAQ</ulink>. É também menos provavél que Linux bloqueie, e corra melhor mais de um programa ao mesmo tempo, e mais suguro que muitos sistemas operativos. Com estas vantagens, Linux é o sistema operativo que cresce mais rapidamente no mercado de servidores. Mais recentemente, Linux passou a ser popular entre os utilizadores domésticos e empresariais. Linux é um sistema operativo: uma série de programas que o deixam interagir com o seu computador e correr outros programas. Linux é moldado no sistema operativo Unix. Desde o ínicio, Linux foi desenhado para ser um sistema multi-tarefa, multi-utilizador. Estes factos são suficientes para tornar Linux diferente de outros sistemas operativos bem conhecidos. No entanto, Linux é mesmo mais diferente do que você possa imaginar. Em contraste com outros sistemas operativos, ningém é dono de Linux. muito do seu desenvolvimento é feito por voluntários não pagos. Os utilizadores de GNU/Linux têm uma imensa liberdade de escolha no seu software. Por exemplo, utilizadores de GNU/Linux podem escolher de entre uma dúzia de shells de linha de comandos e vários ambientes gráficos. Esta selecção é muitas vezes confusa para os utilizadores de outros sistemas operativos, que não estão habituados a pensarem na linha de comandos ou no ambiente de trabalho em algo que possam substituir. Muitos dos programas no sistema são licenciados sob a <emphasis>GNU</emphasis> <emphasis>General Public License</emphasis>, muitas vezes referida como a <quote>GPL</quote>. A GPL requer que você torne o <emphasis>código fonte</emphasis> dos programas disponível quando você distribuir uma cópia do binário do programa; essa medida da licença assegura que qualquer utilizador possa modificar o software. Devido a esta medida, o código fonte para todos esses programas estão disponíveis no sistema Debian. Note que o Debian Project, como concessão pragmática aos seus utilizadores, torna disponíveis alguns pacotes que não seguem os nossos critérios de liberdade. Esses pacotes não são parte da distribuição oficial, no entanto, estão apenas disponíveis das áreas <userinput>contrib</userinput> ou <userinput>non-free</userinput> dos mirrors Debian ou em CDs de terceiros; veja o <ulink url="&url-debian-faq;">Debian FAQ</ulink>, sob <quote>os arquivos The Debian FTP</quote>, para mais informação acerca da disposição e conteúdo dos arquivos. Assim que tiver o seu sistema instalado, você pode ler <xref linkend="post-install"/>. Esse capítulo explica onde pode encontrar mais informação sobre Debian e Unix, e como substituir o seu kernel. Organização Deste Documento Faça a instalação de acordo com <xref linkend="d-i-intro"/>. Isto involve escolher o seu idioma, configurar módulos de controladores de periféricos, configurar a ligação de rede, de modo a que os restantes ficheiros de instalação possam ser obtidos directamente de um servidor Debian (isto se não estiver a instalar por CD), particionar os seus discos rígidos e instalar um sistema minimamente funcional. (Algumas noções de como fazer o particionamento no sistema Debian são explicadas em <xref linkend="partitioning"/>.) O código fonte também está disponível publicamente; veja em <xref linkend="administrivia"/> para mais informação sobre como contribuir. Nós agradecemos sugestões, comentários, patches, e relatórios de erros (utilize o pacote &d-i-manual; para bugs, mas verifique primeiro se o erro já foi relatado). O <ulink url="&url-debian-policy;">Debian Policy Manual</ulink> é uma especificação extensiva dos standards de qualidade do Debian Project. As <ulink url="&url-dfsg;">Debian Free Software Guidelines</ulink> são uma afirmação clara e concisa acerca dos critérios de Debian para o software livre. A DFSG é um documento muito influente no movimento de software livre, e foi a fundação para a <ulink url="&url-osd;">The Open Source Definition</ulink>. O <ulink url="&url-fhs-home;">Filesystem Hierarchy Standard</ulink> (FHS) é um esforço para estandardizar a organização do sistema de ficheiros em Linux. O FHS vai permitir aos criadores de software concentrarem os seus esforços em problemas de design, sem terem de se preocupar como o pacote irá ser instalado nas diferentes distribuições de GNU/Linux. O <ulink url="&url-kernel-org;">kernel Linux</ulink> apareceu pela primeira vez em 1991, quando um estudante Finlandês de ciência computacional anunciou uma versão prévia de um kernel de substituto de Minix num newsgroup Usenet <userinput>comp.os.minix</userinput>. Veja a <ulink url="&url-linux-history;">Linux History Page</ulink> da Linux International. A <ulink url="&url-lsb-org;">Linux Standard Base</ulink> (LSB) é um projecto orientado para a standardização do sistem básico GNU/Linux, a qual permite a outros criadores de software e de hardware facilmente desenhar programas e controladores de dispositivos para Linux em geral, em vez de o fazerem para uma distribuição específica de GNU/Linux. O <ulink url="&url-social-contract;">Debian Social Contract</ulink> inclui a afirmação do que Debian se compromete perante a comunidade de software livre. Quem quer que aceite seguir o Social Contract pode tornar-se um <ulink url="&url-new-maintainer;">maintainer</ulink>. Qualquer maintainer pode introduzir novo software em Debian &mdash; desde que o software siga os nossos critérios acerca de ser livre, e o pacote siga os nossos standards de qualidade. O GNU Project desenvolveu uma extensa lista de ferramentas de software livre para utilizar com Unix&trade; e sistemas operativos do tipo Unix como o Linux. Estas ferramentas permitem aos utilizadores executar tarefas que vão desde o mundano (como copiar ou remover ficheiros do sistema) ao arcano (como escrever e compilar programas ou editar de forma sofisticada numa variedade de formatos de documentos). A combinação da filosofia e metodologia Debian e as ferramentas GNU, o kernel Linux, e outro importante software livre, formam uma distribuição de software única chamada &debian;. Esta distribuição é feita de um grande número de <emphasis>pacotes</emphasis> de software. Cada pacote da distribuição contém executáveis, scripts, documentação, e informação de configuração, e tem um <emphasis>maintainer</emphasis> que é o primeiro responsável por manter o pacote actualizado, perseguir relatórios de bugs, e comunicar com o(s) autor(es) do software original do pacote. A nossa extremamente grande base de utilizadores, combinada com o nosso Bug Tracking System assegura que os problemas são encontrados e resolvidos rapidamente. A funcionalidade que mais distingue Debian de outras distribuições de Linux é o sistema de gestão de pacotes. Estas ferramentas dão ao administrador de um sistema Debian o controlo completo sobre os pacotes instalados nesse sistema, incluindo a possibilidade de instalar um único pacote ou actualizar automaticamente todo o sistema operativo. Pacotes individuais podem também ser protegidos para não serem actualizados. Você pode mesmo dizer ao sistema de gestão de pacotes que software compilou você mesmo e que dependências satisfaz. O aviso legal mais importante é que o software vem <emphasis>sem nenhuma garantia</emphasis>. Os programadores que criaram este software fizeram-no em benefício da comunidade. Nenhumas garantias são feitas acerca da adequação do software para um determinado propósito. No entanto, como o software é livre, você está autorizado a modificar o software para adaptá-lo às suas necessidades &mdash; e para gozar dos benefícios das alterações feitas por outros que extenderam o software dessa forma. O principal, e melhor, método para obter suporte para o seu sistema &debian; é comunicar com Debian Developers através das muitas mailing lists mantidas pelo Debian Project (existem mais de &num-of-debian-maillists; quando isto foi escrito). A forma mais fácil de subscrever uma ou mais destas mailing lists é visitar a <ulink url="&url-debian-lists-subscribe;"> página de subscrição de mailing lists Debian</ulink> e preencher o formulário que vai lá encontrar. Existem algumas outras formas de afirmação de direitos de cópia e licenças de software utilizadas nos programas em Debian. Você pode encontrar os direitos de cópia e licenças para cada pocote instalado no seu sistema simplesmente vendo o ficheiro <filename>/usr/share/doc/<replaceable>package-name</replaceable>/copyright</filename> depois de instalar um pacote no seu sistema. Este capítulo pretende dar a conhecer, de uma forma geral, o Debian Project e &debian;. Se já conhece a história do Debian Project e a distribuição &debian;, pode tomar a liberdade de avançar para o próximo capítulo. Este documento está constantemente a ser revisto. Assegure-se de que verifica as <ulink url="&url-release-area;">páginas de Debian &release;</ulink> para verificar a existência de informação de última hora acerca do lançamento do sistema &debian; &release;. Versões mais actualizadas deste manual estão também disponíveis nas <ulink url="&url-install-manual;">páginas do Manual Oficial de Instalação</ulink>. Este documento tem o propósito de servir como um manual para os utilizadores pela primeira vez de Debian. Tenta fazer o minimo de suposições possíveis acerca do seu nível de perícia. No entanto, nós assumimos de que você tem um bom entendimento geral acerca de como trabalha o hardware no seu computador. Para proteger o seu sistema contra <quote>Cavalos de Tróia</quote> e outro software malévolo, os servidores Debian verificam se os pacotes lá colocados provêm dos seus maintainers Debian registados. Os empacotadores Debian também têm bastante cuidado a configurar os seus pacotes de uma forma segura. Quando apareçem problemas de segurança em pacotes lançados, as correcções geralmente estão disponíveis muito rapidamente. Com a simplicidade das opções de actualização, as correcções de segurança podem ser obtidas e instaladas automaticamente a partir da Internet. Nós temos a certeza que você já leu as licenças que vêm com a maioria do software &mdash; estas geralmente dizem que você só pode utilizar uma cópia do software num único computador. A licença deste sistema não é nada como essas. Nós encorajamo-lo a colocar uma cópia em cada computador da sua escola ou local de trabalho. Empreste o meio de instalação aos seus amigos e ajude-os a instalar nos seus computadores! Você pode mesmo fazer milhares de cópias e <emphasis>vende-las</emphasis>&mdash; embora com algumas restrições. A sua liberdade de instalar e utilizar o sistema vem directamente de Debian ser baseado em <emphasis>software livre</emphasis>. Benvindo a Debian O que é &debian;? O que é Debian GNU/Hurd? O que é Debian? O que é GNU/Linux? Enquanto que muitos grupos e indivíduos contribuíram para GNU/Linux, o maior contribuidor individual continua a ser a Free Software Foundation, que criou não só a maioria das ferramentas utilizadas em GNU/Linux, mas também a filosofia e a comunidade que tornaram GNU/Linux possível. A Sua Ajuda na Documentação é Benvinda descreve o arranque para o sistema de instalação. Este capítulo também discute procedimentos no caso de problemas com esta etapa. 